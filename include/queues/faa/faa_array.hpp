#ifndef LOO_QUEUE_BENCHMARK_FAA_ARRAY_HPP
#define LOO_QUEUE_BENCHMARK_FAA_ARRAY_HPP

#include "faa_array_fwd.hpp"
#include "queues/faa/detail/node.hpp"

namespace faa {
template <typename T, detail::queue_variant_t V>
queue<T, V>::queue(std::size_t max_threads) : m_hazard_ptrs{ max_threads, 1 } {
  auto head = new node_t();
  this->m_head.store(head, relaxed);
  this->m_tail.store(head, relaxed);
}

template <typename T, detail::queue_variant_t V>
queue<T, V>::~queue() noexcept {
  auto curr = this->m_head.load(relaxed);
  while (curr != nullptr) {
    const auto next = curr->next.load(relaxed);
    delete curr;
    curr = next;
  }
}

template <typename T, detail::queue_variant_t V>
void queue<T, V>::enqueue(queue::pointer elem, std::size_t thread_id) {
  if (elem == nullptr) [[unlikely]] {
    throw std::invalid_argument("enqueue element must not be null");
  }

  while (true) {
    const auto tail = this->m_hazard_ptrs.protect_ptr(
        this->m_tail.load(relaxed),
        thread_id, HP_ENQ_TAIL
    );

    if (tail != this->m_tail.load(acquire)) [[unlikely]] {
      continue;
    }

    const auto idx = tail->enq_idx.fetch_add(1, relaxed);
    if (idx < NODE_SIZE) [[likely]] {
      // ** fast path ** write (CAS) pointer directly into the reserved slot
      if (tail->cas_slot_at(idx, nullptr, elem, release)) [[likely]] {
        break;
      }

      continue;
    } else {
      // ** slow path ** append new tail node or update the tail pointer
      if (tail != this->m_tail.load(relaxed)) {
        continue;
      }

      const auto next = tail->next.load(acquire);
      if (next == nullptr) {
        auto node = new node_t(elem);
        if (tail->cas_next(nullptr, node, release)) {
          this->cas_tail(tail, node, release);
          break;
        }

        delete node;
      } else {
        this->cas_tail(tail, next, release);
      }
    }
  }

  this->m_hazard_ptrs.clear_one(thread_id, HP_ENQ_TAIL);
}

template <typename T, detail::queue_variant_t V>
typename queue<T, V>::pointer queue<T, V>::dequeue(std::size_t thread_id) {
  pointer res = nullptr;
  while (true) {
    // acquire hazard pointer for head node
    const auto head = this->m_hazard_ptrs.protect_ptr(
        this->m_head.load(relaxed),
        thread_id, HP_DEQ_HEAD
    );

    if (head != this->m_head.load(acquire)) [[unlikely]] {
      continue;
    }

    // prevent incrementing dequeue index in case the queue is empty
    if (this->is_empty(head)) {
      break;
    }

    // increment the dequeue index to reserve an array slot
    const auto idx = head->deq_idx.fetch_add(1, relaxed);
    if (idx < NODE_SIZE) [[likely]] {
      // ** fast path ** read the pointer from the reserved slot
      res = head->slots[idx].exchange(reinterpret_cast<pointer>(TAKEN), acquire);
      if (res != nullptr) [[likely]] {
        break;
      }

      // abandon the slot and attempt to dequeue from another slot
      continue;
    } else {
      // ** slow path ** advance the head pointer to the next node
      const auto next = head->next.load(acquire);
      if (next == nullptr) {
        break;
      }

      if (this->cas_head(head, next, release)) {
        this->m_hazard_ptrs.retire(head, thread_id);
      }

      continue;
    }
  }

  this->m_hazard_ptrs.clear_one(thread_id, HP_DEQ_HEAD);
  return res;
}

template <typename T, detail::queue_variant_t V>
bool queue<T, V>::is_empty(queue::node_t* head) {
  if constexpr (V == detail::queue_variant_t::ORIGINAL) {
    return
      head->deq_idx.load(relaxed) >= head->enq_idx.load(acquire)
      && head->next.load(acquire) == nullptr;
  } else if constexpr (V == detail::queue_variant_t::VARIANT_1) {
    return
      head->enq_idx.load(relaxed) <= head->deq_idx.load(acquire)
      && head->next.load(acquire) == nullptr;
  } else if constexpr (V == detail::queue_variant_t::VARIANT_2) {
    return
        head->enq_idx.load(relaxed) <= head->deq_idx.fetch_add(0, acquire)
        && head->next.load(acquire) == nullptr;
  } else {
    return
        head->deq_idx.fetch_add(0, relaxed) >= head->enq_idx.load(acquire)
        && head->next.load(acquire) == nullptr;
  }
}

template <typename T, detail::queue_variant_t V>
bool queue<T, V>::cas_head(
    queue::node_t* expected, queue::node_t* desired, std::memory_order order
) {
  return this->m_head.compare_exchange_strong(
      expected, desired, order, relaxed
  );
}

template <typename T, detail::queue_variant_t V>
bool queue<T, V>::cas_tail(
    queue::node_t* expected, queue::node_t* desired, std::memory_order order
) {
  return this->m_tail.compare_exchange_strong(
      expected, desired, order, relaxed
  );
}
}

#endif /* LOO_QUEUE_BENCHMARK_FAA_ARRAY_HPP */
