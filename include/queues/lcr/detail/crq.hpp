#ifndef LOO_QUEUE_BENCHMARK_LCRQ_DETAIL_CRQ_HPP
#define LOO_QUEUE_BENCHMARK_LCRQ_DETAIL_CRQ_HPP

#include "queues/lcr/lcrq_fwd.hpp"

#include <array>
#include <atomic>
#include <stdexcept>

#include "looqueue/align.hpp"

namespace lcr {
namespace detail {
template<typename T>
struct cell_t {
  std::uint64_t idx;
  T* ptr;
};

template<typename T>
struct alignas(CACHE_LINE_SIZE) atomic_cell_t {
  using cell_t  = detail::cell_t<T>;

  std::atomic_uint64_t idx;
  std::atomic<T*> ptr{ nullptr };

  bool compare_exchange_weak(
      cell_t& expected,
      cell_t desired,
      std::memory_order success = std::memory_order_seq_cst,
      std::memory_order failure = std::memory_order_seq_cst
  ) {
    (void) success;
    (void) failure;
    std::uint8_t res;
    asm volatile(
      "lock cmpxchg16b %0"
      : "+m"(*this), "=@ccz"(res), "+a"(expected.idx), "+d"(expected.ptr)
      : "b"(desired.idx), "c"(desired.ptr)
      : "memory"
    );
    return res != 0;
  }
};
}

template <typename T>
class queue<T>::crq_t {
  /** type aliases */
  using cell_t        = detail::cell_t<T>;
  using atomic_cell_t = detail::atomic_cell_t<T>;
  /** status bits and mask */
  static constexpr auto STATUS_BIT = std::uint64_t{ 1 } << std::uint64_t { 63 };
  static constexpr auto INDEX_MASK = ~STATUS_BIT;
  static constexpr auto PATIENCE   = 10;
  /** decomposed status bit and index value */
  struct decomposed_idx_t;

  void init_cells() noexcept {
    for (std::size_t idx = 0; idx < RING_SIZE; ++idx) {
      this->m_cells[idx].idx.store(STATUS_BIT | idx, relaxed);
    }
  }

  alignas(CACHE_LINE_SIZE) std::atomic_uint64_t m_head_ticket;
  alignas(CACHE_LINE_SIZE) std::atomic_uint64_t m_tail_ticket;
  alignas(CACHE_LINE_SIZE) std::array<atomic_cell_t, RING_SIZE> m_cells{ };

public:
  /** constructors */
  crq_t() noexcept;
  explicit crq_t(pointer first);
  /** destructor */
  ~crq_t() noexcept = default;

  bool try_enqueue(pointer elem) noexcept;
  bool try_dequeue(pointer& result) noexcept;
  void fix_state();

  crq_t(const crq_t&)                      = delete;
  crq_t(crq_t&&) noexcept                  = delete;
  const crq_t& operator=(const crq_t&)     = delete;
  const crq_t& operator=(crq_t&&) noexcept = delete;
};

template <typename T>
struct queue<T>::crq_t::decomposed_idx_t {
  explicit decomposed_idx_t(std::uint64_t val) :
      status{ STATUS_BIT & val }, idx{ val & INDEX_MASK } {}
  decomposed_idx_t(std::uint64_t status, std::uint64_t idx) :
    status{ status }, idx{ idx } {}

  [[nodiscard]]
  std::uint64_t compose() const {
    return this->status | this->idx;
  }

  std::uint64_t status, idx;
};

template <typename T>
queue<T>::crq_t::crq_t() noexcept : m_head_ticket{ 0 }, m_tail_ticket{ 0 } {
  this->init_cells();
}

template <typename T>
queue<T>::crq_t::crq_t(pointer first) : m_head_ticket{ 0 }, m_tail_ticket{ 1 } {
  if (first == nullptr) {
    throw std::invalid_argument("enqueue element must not be null");
  }

  this->m_cells[0].ptr.store(first, relaxed);
  this->init_cells();
}

template <typename T>
bool queue<T>::crq_t::try_enqueue(pointer elem) noexcept {
  auto attempts = 0;
  while (true) {
    const auto [is_closed, tail_ticket] = decomposed_idx_t{ this->m_tail_ticket.fetch_add(1) };
    if (is_closed == STATUS_BIT) {
      return false;
    }

    auto& cell = this->m_cells[tail_ticket % RING_SIZE];
    auto ptr = cell.ptr.load();

    const auto composed_idx = cell.idx.load();
    const auto [is_safe, idx] = decomposed_idx_t{ composed_idx };

    if (ptr == nullptr) {
      if(
          idx <= tail_ticket
          && (
              is_safe == STATUS_BIT
              || this->m_head_ticket.load() <= tail_ticket
          )
      ) {
        auto expected = cell_t{ composed_idx, nullptr };
        const auto desired = cell_t{
            decomposed_idx_t{ STATUS_BIT, tail_ticket }.compose(),
            elem
        };

        if (cell.compare_exchange_weak(expected, desired)) {
          return true;
        }
      }
    }

    auto head_ticket = this->m_head_ticket.load();
    const auto cmp =
        static_cast<std::int64_t>(tail_ticket) -
        static_cast<std::int64_t>(head_ticket) >= RING_SIZE;
    if (cmp || attempts >= PATIENCE) {
      this->m_tail_ticket.fetch_or(STATUS_BIT);
      return false;
    }

    attempts += 1;
  }
}

template <typename T>
bool queue<T>::crq_t::try_dequeue(pointer& result) noexcept {
  while (true) {
    const auto head_ticket = this->m_head_ticket.fetch_add(1);
    auto& cell = this->m_cells[head_ticket % RING_SIZE];

    while (true) {
      auto ptr = cell.ptr.load();
      const auto composed_idx = cell.idx.load();
      const auto [is_safe, idx] = decomposed_idx_t{ composed_idx };

      if (idx > head_ticket) {
        break;
      }

      if (ptr != nullptr) {
        if (idx == head_ticket) {
          // attempt dequeue transition
          auto expected = cell_t{ decomposed_idx_t{ is_safe, head_ticket }.compose(), ptr };
          const auto desired = cell_t{
              decomposed_idx_t{ is_safe, head_ticket + RING_SIZE }.compose(),
              nullptr
          };

          if (cell.compare_exchange_weak(expected, desired)) {
            result = ptr;
            return true;
          }
        } else {
          // mark cell unsafe to prevent future enqueue
          auto expected = cell_t{ composed_idx, ptr };
          const auto desired = cell_t{ decomposed_idx_t{ 0, idx }.compose(), ptr };

          if (cell.compare_exchange_weak(expected, desired)) {
            break;
          }
        }
      } else { // attempt empty transition (idx <= head_ticket and ptr == nullptr)
        auto expected = cell_t{ composed_idx, nullptr };
        const auto desired = cell_t{
            decomposed_idx_t{ is_safe, head_ticket + RING_SIZE }.compose(),
            nullptr
        };

        if (cell.compare_exchange_weak(expected, desired)) {
          break;
        }
      }
    }

    // dequeue failed, check for empty
    const auto tail_ticket = decomposed_idx_t{ this->m_tail_ticket.load() }.idx;
    if (tail_ticket <= head_ticket + 1) {
      this->fix_state();
      return false;
    }
  }
}

template <typename T>
void queue<T>::crq_t::fix_state() {
  while (true) {
    auto tail_ticket = this->m_tail_ticket.fetch_add(0);
    const auto head_ticket = this->m_head_ticket.fetch_add(0);

    if (this->m_tail_ticket.load() != tail_ticket) {
      continue;
    }

    if (head_ticket <= tail_ticket) {
      return;
    }

    if (this->m_tail_ticket.compare_exchange_strong(tail_ticket, head_ticket)) {
      return;
    }
  }
}
}

#endif /* LOO_QUEUE_BENCHMARK_LCRQ_DETAIL_CRQ_HPP */
