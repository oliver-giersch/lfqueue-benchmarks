#ifndef LOO_QUEUE_BENCHMARK_MICHAEL_SCOTT_FWD_HPP
#define LOO_QUEUE_BENCHMARK_MICHAEL_SCOTT_FWD_HPP

#include <atomic>

#include "hazard_pointers/hazard_pointers.hpp"
#include "looqueue/align.hpp"
#include "queues/queue_ref.hpp"

namespace msc {
template <typename T>
class queue {
public:
  using pointer = T*;

  explicit queue(std::size_t max_threads = MAX_THREADS);
  ~queue() noexcept;
  void enqueue(pointer elem, std::size_t thread_id);
  pointer dequeue(std::size_t thread_id);

  queue(const queue&)            = delete;
  queue(queue&&)                 = delete;
  queue& operator=(const queue&) = delete;
  queue& operator=(queue&&)      = delete;

private:
  static constexpr std::size_t MAX_THREADS = 128;

  /** enqueue and dequeue use the same hazard pointer */
  static constexpr std::size_t HP_ENQ_TAIL = 0;
  static constexpr std::size_t HP_DEQ_HEAD = 0;
  static constexpr std::size_t HP_DEQ_NEXT = 1;
  /** ordering constants */
  static constexpr auto relaxed = std::memory_order_relaxed;
  static constexpr auto acquire = std::memory_order_acquire;
  static constexpr auto release = std::memory_order_release;

  struct node_t {
    bool cas_next(node_t*& curr, node_t* next_node, std::memory_order order);

    pointer elem;
    std::atomic<node_t*> next{ nullptr };
  };

  bool cas_head(node_t* curr, node_t* next, std::memory_order order);
  bool cas_tail(node_t* curr, node_t* next, std::memory_order order);

  alignas(CACHE_LINE_ALIGN) std::atomic<node_t*> m_head;
  alignas(CACHE_LINE_ALIGN) std::atomic<node_t*> m_tail;
  alignas(CACHE_LINE_ALIGN) memory::hazard_pointers<node_t> m_hazard_pointers;
};

template <typename T>
using queue_ref = queue_ref<queue<T>>;
}

#endif /* LOO_QUEUE_BENCHMARK_MICHAEL_SCOTT_FWD_HPP */
