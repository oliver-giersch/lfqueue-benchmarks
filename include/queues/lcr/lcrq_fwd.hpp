#ifndef LOO_QUEUE_BENCHMARK_LCRQ_FWD_HPP
#define LOO_QUEUE_BENCHMARK_LCRQ_FWD_HPP

#include <atomic>

#include "hazard_pointers/hazard_pointers.hpp"
#include "looqueue/align.hpp"

namespace lcr {
template <typename T>
/** Implementation of (L)CRQ by Morrison & Afek. */
class queue {
  static constexpr std::size_t RING_SIZE   = 1024;
  static constexpr std::size_t MAX_THREADS = 128;
  /** enqueue and dequeue use the same hazard pointer */
  static constexpr std::size_t HP_ENQ_TAIL = 0;
  static constexpr std::size_t HP_DEQ_HEAD = 0;
  /** ordering constants */
  static constexpr auto relaxed = std::memory_order_relaxed;
  static constexpr auto acquire = std::memory_order_acquire;
  static constexpr auto release = std::memory_order_release;

  struct crq_node_t;
  class crq_t;

  using hazard_pointers_t = memory::hazard_pointers<crq_node_t>;

  alignas(CACHE_LINE_ALIGN) std::atomic<crq_node_t*> m_head;
  alignas(CACHE_LINE_ALIGN) std::atomic<crq_node_t*> m_tail;
  alignas(CACHE_LINE_ALIGN) hazard_pointers_t        m_hazard_pointers;

public:
  using pointer = T*;

  /** constructor */
  explicit queue(std::size_t max_threads = MAX_THREADS);
  /** destructor */
  ~queue() noexcept;

  void enqueue(pointer elem, std::size_t thread_id);
  pointer dequeue(std::size_t thread_id);

  queue(const queue&)             = delete;
  queue(queue&&)                  = delete;
  queue& operator=(const queue&&) = delete;
  queue& operator=(queue&&)       = delete;
};
}

#endif /* LOO_QUEUE_BENCHMARK_LCRQ_FWD_HPP */
