#ifndef LOO_QUEUE_BENCHMARK_MICHAEL_SCOTT_HPP
#define LOO_QUEUE_BENCHMARK_MICHAEL_SCOTT_HPP

#include "michael_scott_fwd.hpp"

namespace msc {
template <typename T>
queue<T>::queue(std::size_t max_threads) :
  m_hazard_pointers{ max_threads, 2, 100 }
{
  const auto sentinel = new node_t{ nullptr };
  this->m_head.store(sentinel, relaxed);
  this->m_tail.store(sentinel, relaxed);
}

template <typename T>
queue<T>::~queue<T>() noexcept {
  auto curr = this->m_head.load(relaxed);
  while (curr != nullptr) {
    auto next = curr->next.load(relaxed);
    delete curr;
    curr = next;
  }
}

template <typename T>
void queue<T>::enqueue(queue::pointer elem, std::size_t thread_id) {
  if (elem == nullptr) {
    throw std::invalid_argument("enqueue element must not be nullptr");
  }

  auto node = new node_t{ elem };
  while (true) {
    auto tail = this->m_hazard_pointers.protect_ptr(
        this->m_tail.load(relaxed), thread_id, HP_ENQ_TAIL
    );

    if (this->m_tail.load(acquire) != tail) continue;

    node_t* expected = nullptr;
    if (tail->cas_next(expected, node, release)) {
      this->cas_tail(tail, node, release);
      break;
    } else {
      this->cas_tail(tail, expected, release);
    }
  }

  this->m_hazard_pointers.clear_one(thread_id, HP_ENQ_TAIL);
}

template <typename T>
typename queue<T>::pointer queue<T>::dequeue(std::size_t thread_id) {
  auto head = this->m_hazard_pointers.protect(this->m_head, thread_id, HP_DEQ_HEAD);

  while (head != this->m_tail.load(acquire)) {
    auto next = this->m_hazard_pointers.protect(head->next, thread_id, HP_DEQ_NEXT);
    if (this->cas_head(head, next, acquire)) {
      auto res = next->elem;
      this->m_hazard_pointers.clear(thread_id);
      this->m_hazard_pointers.retire(head, thread_id);

      return res;
    }
    head = this->m_hazard_pointers.protect(this->m_head, thread_id, HP_DEQ_HEAD);
  }

  this->m_hazard_pointers.clear(thread_id);
  return nullptr;
}

template <typename T>
bool queue<T>::cas_head(queue::node_t* curr, queue::node_t* next, std::memory_order order) {
  return this->m_head.compare_exchange_strong(curr, next, order, relaxed);
}

template <typename T>
bool queue<T>::cas_tail(queue::node_t* curr, queue::node_t* next, std::memory_order order) {
  return this->m_tail.compare_exchange_strong(curr, next, order, relaxed);
}

template <typename T>
bool queue<T>::node_t::cas_next(
    queue::node_t*& curr, queue::node_t* next_node, std::memory_order order
) {
  return this->next.compare_exchange_strong(curr, next_node, order, relaxed);
}
}

#endif /* LOO_QUEUE_BENCHMARK_MICHAEL_SCOTT_HPP */
