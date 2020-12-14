#ifndef LOO_QUEUE_BENCHMARK_FAA_ARRAY_FWD_HPP
#define LOO_QUEUE_BENCHMARK_FAA_ARRAY_FWD_HPP

#include <atomic>

#include "hazard_pointers/hazard_pointers.hpp"
#include "looqueue/align.hpp"
#include "queues/queue_ref.hpp"

namespace faa {
namespace detail {
  enum class queue_variant_t { ORIGINAL, VARIANT_1, VARIANT_2, VARIANT_3 };
}

/** Implementation of FAAArrayQueue by Correia & Ramalhete. */
template <typename T, detail::queue_variant_t V = detail::queue_variant_t::ORIGINAL>
class queue {
  /** queue node size and thread limit */
  static constexpr std::size_t NODE_SIZE   = 1024;
  static constexpr std::size_t MAX_THREADS = 128;
  /** enqueue and dequeue use the same hazard pointer */
  static constexpr std::size_t HP_ENQ_TAIL = 0;
  static constexpr std::size_t HP_DEQ_HEAD = 0;
  /** token value for slots dequeued from */
  static constexpr std::size_t TAKEN = 0x1;
  /** ordering constants */
  static constexpr auto relaxed = std::memory_order_relaxed;
  static constexpr auto acquire = std::memory_order_acquire;
  static constexpr auto release = std::memory_order_release;

  struct node_t;

  using hazard_pointers_t = memory::hazard_pointers<node_t>;

  bool is_empty(node_t* head);
  bool cas_head(node_t* curr, node_t* next, std::memory_order order);
  bool cas_tail(node_t* curr, node_t* next, std::memory_order order);

  alignas(CACHE_LINE_ALIGN) std::atomic<node_t*> m_head;
  alignas(CACHE_LINE_ALIGN) std::atomic<node_t*> m_tail;
  alignas(CACHE_LINE_ALIGN) hazard_pointers_t    m_hazard_ptrs;

public:
  using pointer = T*;

  /** constructor */
  explicit queue(std::size_t max_threads = MAX_THREADS);
  ~queue() noexcept;
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
using queue_ref_v1 = ::queue_ref<queue<T, detail::queue_variant_t::VARIANT_1>>;

template <typename T>
using queue_ref_v2 = ::queue_ref<queue<T, detail::queue_variant_t::VARIANT_2>>;

template <typename T>
using queue_ref_v3 = ::queue_ref<queue<T, detail::queue_variant_t::VARIANT_3>>;
}

#endif /* LOO_QUEUE_BENCHMARK_FAA_ARRAY_FWD_HPP */
