#ifndef LFQUEUE_BENCHMARKS_CHASE_LEV_FWD_HPP
#define LFQUEUE_BENCHMARKS_CHASE_LEV_FWD_HPP

#include <atomic>
#include <cstdint>

#include "hazard_pointers/hazard_pointers.hpp"

namespace cld {
namespace detail {} /* detail */

enum class steal_res_t : std::uint8_t {
  Success,
  Empty,
  Retry,
};

template <typename T>
class deque {
  struct node_t;

  using hazard_pointers_t = memory::hazard_pointers<node_t>;

  alignas(64) std::atomic<std::int64_t> m_bottom;
  alignas(64) std::atomic<std::int64_t> m_top;
  alignas(64) std::atomic<node_t*>      m_active
  alignas(64) hazard_pointers_t         m_hazard_ptrs;

public:
  using pointer = T*;

  explicit deque();
  ~queue() noexcept;
  void push_bottom(pointer elem, std::size_t thread_id);
  pointer pop_bottom(std::size_t thread_id);
  steal_res_t steal(pointer& elem, std::size_t thread_id);

  deque(const deque&)            = delete;
  deque(queue&&)                 = delete;
  deque& operator=(const deque&) = delete;
  deque& operator=(deque&&)      = delete;
};
} /* cld */

#endif /* LFQUEUE_BENCHMARKS_CHASE_LEV_HPP */