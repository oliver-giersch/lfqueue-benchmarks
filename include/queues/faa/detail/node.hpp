#ifndef LOO_QUEUE_BENCHMARK_FAA_ARRAY_NODE_HPP
#define LOO_QUEUE_BENCHMARK_FAA_ARRAY_NODE_HPP

#include "queues/faa/faa_array_fwd.hpp"

#include <cstdint>
#include <atomic>
#include <array>

#include "looqueue/align.hpp"

namespace faa {
template <typename T, detail::queue_variant_t V>
struct queue<T, V>::node_t {
  using slot_array_t = std::array<std::atomic<queue::pointer>, queue::NODE_SIZE>;

  std::atomic<std::uint32_t> deq_idx{ 0 };
  slot_array_t               slots{ };
  std::atomic<std::uint32_t> enq_idx{ 0 };
  std::atomic<node_t*>       next{ nullptr };

  node_t() {
    this->init_slots();
  }

  explicit node_t(queue::pointer first) : enq_idx{ 1 } {
    this->init_slots();
    this->slots[0].store(first, std::memory_order_relaxed);
  }

  bool cas_slot_at(
      std::size_t idx,
      queue::pointer expected,
      queue::pointer desired,
      std::memory_order order
  ) {
    return this->slots[idx].compare_exchange_strong(
        expected, desired, order, std::memory_order_relaxed
    );
  }

  bool cas_next(node_t* expected, node_t* desired, std::memory_order order) {
    return this->next.compare_exchange_strong(
        expected, desired, order, std::memory_order_relaxed
    );
  }

private:
  void init_slots() {
    for (auto& slot : this->slots) {
      slot.store(nullptr, std::memory_order_relaxed);
    }
  }
};
}

#endif /* LOO_QUEUE_BENCHMARK_FAA_ARRAY_NODE_HPP */
