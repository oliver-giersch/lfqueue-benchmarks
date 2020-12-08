#ifndef LOO_QUEUE_BENCHMARK_LSCQ_HPP
#define LOO_QUEUE_BENCHMARK_LSCQ_HPP

#include <atomic>
#include <stdexcept>

#include "hazard_pointers/hazard_pointers.hpp"
#include "scqueue/scq.hpp"

namespace lsc {
template <typename T>
class queue {
public:
  using pointer = T*;

  explicit queue(std::size_t max_threads = MAX_THREADS) :
    m_hazard_pointers{ max_threads, 1 }
  {
    auto head = new node_t{};
    this->m_head.store(head, std::memory_order_relaxed);
    this->m_tail.store(head, std::memory_order_relaxed);
  }

  ~queue() noexcept {
    auto curr = this->m_head.load(std::memory_order_relaxed);
    while (curr != nullptr) {
      const auto next = curr->next.load(std::memory_order_relaxed);
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

private:
  static constexpr std::size_t MAX_THREADS = 128;

  /** enqueue and dequeue use the same hazard pointer */
  static constexpr std::size_t HP_ENQ_TAIL = 0;
  static constexpr std::size_t HP_DEQ_HEAD = 0;
  /** ordering constants */
  static constexpr std::memory_order RLX = std::memory_order_relaxed;
  static constexpr std::memory_order ACQ = std::memory_order_acquire;
  static constexpr std::memory_order REL = std::memory_order_release;

  struct node_t {
    node_t() = default;
    explicit node_t(pointer first): ring{ first } {}

    scq::ring<T, 10> ring{ };
    std::atomic<node_t*> next{ nullptr };
  };

  alignas(CACHE_LINE_ALIGN) std::atomic<node_t*> m_head{};
  alignas(CACHE_LINE_ALIGN) std::atomic<node_t*> m_tail{};
  alignas(CACHE_LINE_ALIGN) memory::hazard_pointers<node_t> m_hazard_pointers;
};

template <typename T>
void queue<T>::enqueue(pointer elem, std::size_t thread_id) {
  if (elem == nullptr) {
    throw std::invalid_argument("enqueue element must not be null");
  }

  while (true) {
    auto tail = this->m_hazard_pointers.protect(this->m_tail.load(ACQ), thread_id, HP_ENQ_TAIL);
    if (tail != this->m_tail.load(RLX)) {
      continue;
    }

    if (auto next = tail->next.load(ACQ); next != nullptr) {
      this->m_tail.compare_exchange_strong(tail, next, REL, RLX);
      continue;
    }

    if (tail->ring.template try_enqueue<true>(elem)) {
      break;
    }

    auto node = new node_t(elem);

    node_t* expected = nullptr;
    if (tail->next.compare_exchange_strong(expected, node, REL, RLX)) {
      this->m_tail.compare_exchange_strong(tail, node, REL, RLX);
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
    auto head = this->m_hazard_pointers.protect_ptr(this->m_head.load(ACQ), thread_id, HP_DEQ_HEAD);
    if (head != this->m_head.load(RLX)) {
      continue;
    }

    if (head->ring.try_dequeue(res)) {
      break;
    }

    if (head->next.load(RLX) == nullptr) {
      res = nullptr;
      break;
    }

    head->ring.reset_threshold(REL);

    if (head->ring.try_dequeue(res)) {
      break;
    }

    auto next = head->next.load(ACQ);
    if (this->m_head.compare_exchange_strong(head, next, REL, RLX)) {
      this->m_hazard_pointers.retire(head, thread_id);
    }
  }

  this->m_hazard_pointers.clear_one(thread_id, HP_DEQ_HEAD);
  return res;
}
}

#endif /* LOO_QUEUE_BENCHMARK_LSCQ_HPP */
