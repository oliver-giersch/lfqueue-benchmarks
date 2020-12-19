#ifndef LOO_QUEUE_BENCHMARK_LSCQ_HPP
#define LOO_QUEUE_BENCHMARK_LSCQ_HPP

#include <atomic>
#include <stdexcept>

#include "hazard_pointers/hazard_pointers.hpp"
#include "scqueue/scq2.hpp"
#include "scqueue/scqd.hpp"
#include "queues/queue_ref.hpp"

namespace scq {
template <typename T, template <typename> typename N>
class queue {
  static constexpr std::size_t MAX_THREADS = 128;
  /** enqueue and dequeue use the same hazard pointer */
  static constexpr std::size_t HP_ENQ_TAIL = 0;
  static constexpr std::size_t HP_DEQ_HEAD = 0;
  /** ordering constants */
  static constexpr auto relaxed = std::memory_order_relaxed;
  static constexpr auto acquire = std::memory_order_acquire;
  static constexpr auto release = std::memory_order_release;

  using node_t            = N<T>;
  using hazard_pointers_t = memory::hazard_pointers<node_t>;

  alignas(CACHE_LINE_ALIGN) std::atomic<node_t*> m_head{};
  alignas(CACHE_LINE_ALIGN) std::atomic<node_t*> m_tail{};
  alignas(CACHE_LINE_ALIGN) hazard_pointers_t    m_hazard_pointers;

public:
  using pointer = T*;
  /** constructor */
  explicit queue(std::size_t max_threads = MAX_THREADS) :
    m_hazard_pointers{ max_threads, 1 }
  {
    auto head = new node_t{};
    this->m_head.store(head, relaxed);
    this->m_tail.store(head, relaxed);
  }
  /** destructor */
  ~queue() noexcept {
    auto curr = this->m_head.load(relaxed);
    while (curr != nullptr) {
      const auto next = curr->next.load(relaxed);
      delete curr;
      curr = next;
    }
  }

  void enqueue(pointer elem, std::size_t thread_id);
  pointer dequeue(std::size_t thread_id);

  queue(const queue&)             = delete;
  queue(queue&&)                  = delete;
  queue& operator=(const queue&&) = delete;
  queue& operator=(queue&&)       = delete;
};

namespace detail {
template <typename T, template <typename, std::size_t> typename BQ>
struct node_t {
  using pointer = T*;
  using bounded_queue_t = BQ<T, 10>;

  static_assert(bounded_queue_t::CAPACITY == 1024);

  node_t() = default;
  explicit node_t(pointer first) : bounded_queue{ first } {}

  bounded_queue_t bounded_queue{ };
  std::atomic<node_t*> next{ nullptr };

  bool cas_next(
      node_t* expected,
      node_t* desired,
      std::memory_order order
  ) {
    return this->next.compare_exchange_strong(
        expected,
        desired,
        order,
        std::memory_order_relaxed
    );
  }
};
}

namespace cas2 {
template <typename T>
using node_t = ::scq::detail::node_t<T, bounded_queue_t>;

template <typename T>
using queue = ::scq::queue<T, node_t>;

template <typename T>
using queue_ref = queue_ref<queue<T>>;
}

namespace d {
template <typename T>
using node_t = ::scq::detail::node_t<T, bounded_queue_t>;

template <typename T>
using queue = ::scq::queue<T, node_t>;

template <typename T>
using queue_ref = queue_ref<queue<T>>;
}

template <typename T, template <typename> typename N>
void queue<T, N>::enqueue(pointer elem, std::size_t thread_id) {
  if (elem == nullptr) {
    throw std::invalid_argument("enqueue element must not be null");
  }

  while (true) {
    auto tail = this->m_hazard_pointers.protect(
        this->m_tail.load(relaxed),
        thread_id, HP_ENQ_TAIL
    );

    if (tail != this->m_tail.load(acquire)) {
      continue;
    }

    if (auto next = tail->next.load(acquire); next != nullptr) {
      this->m_tail.compare_exchange_strong(tail, next, release, relaxed);
      continue;
    }

    if (tail->bounded_queue.template try_enqueue<true>(elem)) {
      break;
    }

    auto node = new node_t{ elem };

    if (tail->cas_next(nullptr, node, release)) {
      this->m_tail.compare_exchange_strong(tail, node, release, relaxed);
      break;
    }

    delete node;
  }

  this->m_hazard_pointers.clear_one(thread_id, HP_ENQ_TAIL);
}

template <typename T, template <typename> typename N>
typename queue<T, N>::pointer queue<T, N>::dequeue(std::size_t thread_id) {
  pointer result;
  while (true) {
    auto head = this->m_hazard_pointers.protect_ptr(
        this->m_head.load(relaxed),
        thread_id, HP_DEQ_HEAD
    );

    if (head != this->m_head.load(acquire)) {
      continue;
    }

    if (head->bounded_queue.try_dequeue(result)) {
      break;
    }

    if (head->next.load(relaxed) == nullptr) {
      result = nullptr;
      break;
    }

    head->bounded_queue.reset_threshold(release);

    if (head->bounded_queue.try_dequeue(result)) {
      break;
    }

    auto next = head->next.load(acquire);
    if (this->m_head.compare_exchange_strong(head, next, release, relaxed)) {
      this->m_hazard_pointers.retire(head, thread_id);
    }
  }

  this->m_hazard_pointers.clear_one(thread_id, HP_DEQ_HEAD);
  return result;
}
}

#endif /* LOO_QUEUE_BENCHMARK_LSCQ_HPP */
