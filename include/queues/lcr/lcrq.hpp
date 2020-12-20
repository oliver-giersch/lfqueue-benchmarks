#ifndef LOO_QUEUE_BENCHMARK_LCRQ_HPP
#define LOO_QUEUE_BENCHMARK_LCRQ_HPP

#include "lcrq_fwd.hpp"

#include <atomic>

#include "queues/lcr/detail/crq.hpp"

namespace lcr {
template <typename T>
struct queue<T>::crq_node_t {
  crq_node_t() = default;
  explicit crq_node_t(pointer first) : ring{ first } {}

  crq_t ring{ };
  std::atomic<crq_node_t*> next{ nullptr };

  bool cas_next(
      crq_node_t* expected,
      crq_node_t* desired,
      std::memory_order order
  ) {
    return this->next.compare_exchange_strong(
        expected, desired, order, relaxed
    );
  }
};

template <typename T>
queue<T>::queue(std::size_t max_threads) :
  m_hazard_pointers{ max_threads }
{
  auto head = new crq_node_t();
  this->m_head.store(head, relaxed);
  this->m_tail.store(head, relaxed);
}

template <typename T>
queue<T>::~queue<T>() noexcept {
  auto curr = this->m_head.load(relaxed);
  while (curr != nullptr) {
    const auto next = curr->next.load(relaxed);
    delete curr;
    curr = next;
  }
}

template <typename T>
void queue<T>::enqueue(queue::pointer elem, std::size_t thread_id) {
  if (elem == nullptr) {
    throw std::invalid_argument("enqueue element must not be null");
  }

  while (true) {
    auto tail = this->m_hazard_pointers.protect_ptr(
        this->m_tail.load(relaxed), thread_id, HP_ENQ_TAIL
    );

    if (tail != this->m_tail.load(acquire)) [[unlikely]] {
      continue;
    }

    if (auto next = tail->next.load(acquire); next != nullptr) {
      this->m_tail.compare_exchange_strong(tail, next, release, relaxed);
      continue;
    }

    if (tail->ring.try_enqueue(elem)) [[likely]] {
      break;
    }

    auto node = new crq_node_t(elem);

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
        this->m_head.load(relaxed),
        thread_id, HP_DEQ_HEAD
    );

    if (head != this->m_head.load(acquire)) {
      continue;
    }

    if (head->ring.try_dequeue(res)) [[likely]] {
      break;
    }

    if (head->next.load(relaxed) == nullptr) {
      res = nullptr;
      break;
    }

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

#endif /* LOO_QUEUE_BENCHMARK_LCRQ_HPP */
