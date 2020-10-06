#ifndef LOO_QUEUE_BENCHMARK_FAA_ARRAY_PLUS_NODE_HPP
#define LOO_QUEUE_BENCHMARK_FAA_ARRAY_PLUS_NODE_HPP

#include "queues/faa+/faa_array_plus_fwd.hpp"

#include <cstdint>
#include <atomic>
#include <array>

namespace faa_plus {
template <typename T>
struct queue<T>::node_t {
  struct /*alignas(CACHE_LINE_SIZE)*/ aligned_slot_t {
    std::atomic<queue::pointer> ptr;
  };

  using slot_array_t = std::array<aligned_slot_t, queue::NODE_SIZE>;

  /*alignas(CACHE_LINE_SIZE)*/ std::atomic<std::uint32_t> deq_idx{ 0 };
  slot_array_t slots;
  /*alignas(CACHE_LINE_SIZE)*/ std::atomic<std::uint32_t> enq_idx{ 0 };
  /*alignas(CACHE_LINE_SIZE)*/ std::atomic<node_t*>       next{ nullptr };

  node_t() {
    this->init_slots();
  }

  explicit node_t(queue::pointer first) : enq_idx{ 1 } {
    this->init_slots();
    this->slots[0].ptr.store(first, std::memory_order_relaxed);
  }

  bool cas_slot_at(std::size_t idx, queue::pointer expected, queue::pointer desired) {
    return this->slots[idx].ptr.compare_exchange_strong(expected, desired);
  }

  bool cas_next(node_t* expected, node_t* desired) {
    return this->next.compare_exchange_strong(expected, desired);
  }

private:
  void init_slots() {
    for (auto& slot : this->slots) {
      slot.ptr.store(nullptr, std::memory_order_relaxed);
    }
  }
};
}

#endif /* LOO_QUEUE_BENCHMARK_FAA_ARRAY_PLUS_NODE_HPP */
