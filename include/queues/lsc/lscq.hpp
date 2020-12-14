#ifndef LOO_QUEUE_BENCHMARK_LSCQ_HPP
#define LOO_QUEUE_BENCHMARK_LSCQ_HPP

#include <atomic>
#include <stdexcept>

#include "hazard_pointers/hazard_pointers.hpp"
#include "scqueue/scq.hpp"
#include "queues/queue_ref.hpp"

namespace lsc {
template <typename T>
class queue {
  static constexpr std::size_t MAX_THREADS = 128;
  /** enqueue and dequeue use the same hazard pointer */
  static constexpr std::size_t HP_ENQ_TAIL = 0;
  static constexpr std::size_t HP_DEQ_HEAD = 0;
  /** ordering constants */
  static constexpr auto relaxed = std::memory_order_relaxed;
  static constexpr auto acquire = std::memory_order_acquire;
  static constexpr auto release = std::memory_order_release;

  struct scq_node_t;

  using hazard_pointers_t = memory::hazard_pointers<scq_node_t>;

  alignas(CACHE_LINE_ALIGN) std::atomic<scq_node_t*> m_head{};
  alignas(CACHE_LINE_ALIGN) std::atomic<scq_node_t*> m_tail{};
  alignas(CACHE_LINE_ALIGN) hazard_pointers_t        m_hazard_pointers;

public:
  using pointer = T*;

  /** constructor */
  explicit queue(std::size_t max_threads = MAX_THREADS) :
    m_hazard_pointers{ max_threads, 1 }
  {
    auto head = new scq_node_t{};
    this->m_head.store(head, relaxed);
    this->m_tail.store(head, relaxed);
  }

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

template <typename T>
using queue_ref = queue_ref<queue<T>>;

template <typename T>
struct queue<T>::scq_node_t {
  using scq_ring_t = typename scq::ring_t<T, 9>;

  scq_node_t() = default;
  explicit scq_node_t(pointer first): ring{ first } {}

  scq_ring_t ring{ };
  std::atomic<scq_node_t*> next{ nullptr };

  bool cas_next(
      scq_node_t* expected, scq_node_t* desired, std::memory_order order
  ) {
    return this->next.compare_exchange_strong(
        expected, desired, order, relaxed
    );
  }
};

template <typename T>
void queue<T>::enqueue(pointer elem, std::size_t thread_id) {
  if (elem == nullptr) {
    throw std::invalid_argument("enqueue element must not be null");
  }

  while (true) {
    auto tail = this->m_hazard_pointers.protect(
        this->m_tail.load(acquire), thread_id, HP_ENQ_TAIL
    );

    if (tail != this->m_tail.load(relaxed)) {
      continue;
    }

    if (auto next = tail->next.load(acquire); next != nullptr) {
      this->m_tail.compare_exchange_strong(tail, next, release, relaxed);
      continue;
    }

    if (tail->ring.template try_enqueue<true>(elem)) {
      break;
    }

    auto node = new scq_node_t(elem);

    if (tail->cas_next(nullptr, node, release)) {
      this->m_tail.compare_exchange_strong(tail, node, release, relaxed);
      break;
    }

    delete node;
  }

  this->m_hazard_pointers.clear_one(thread_id, HP_ENQ_TAIL);
}

template <typename T>
typename queue<T>::pointer queue<T>::dequeue(std::size_t thread_id) {
  pointer res;
  while (true) {
    auto head = this->m_hazard_pointers.protect_ptr(
        this->m_head.load(acquire), thread_id, HP_DEQ_HEAD
    );

    if (head != this->m_head.load(relaxed)) {
      continue;
    }

    if (head->ring.try_dequeue(res)) {
      break;
    }

    if (head->next.load(relaxed) == nullptr) {
      res = nullptr;
      break;
    }

    head->ring.reset_threshold(release);

    if (head->ring.try_dequeue(res)) {
      break;
    }

    auto next = head->next.load(acquire);
    if (this->m_head.compare_exchange_strong(head, next, release, relaxed)) {
      this->m_hazard_pointers.retire(head, thread_id);
    }
  }

  this->m_hazard_pointers.clear_one(thread_id, HP_DEQ_HEAD);
  return res;
}
}

#endif /* LOO_QUEUE_BENCHMARK_LSCQ_HPP */
