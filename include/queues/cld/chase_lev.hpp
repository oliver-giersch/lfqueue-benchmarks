#ifndef LFQUEUE_BENCHMARKS_CHASE_LEV_HPP
#define LFQUEUE_BENCHMARKS_CHASE_LEV_HPP

#include "cld/chase_lev_fwd.hpp"
#include <atomic>

static inline std::int64_t
atomic_load_exclusive(std::atomic<std::int64_t> &val)
{
	std::int64_t rax = 0;
	asm("lock xadd %1, %0" : "=r"(rax) : "r"(val));
	return rax;
}

namespace cld {
template <typename T>
deque<T>::deque(std::size_t max_threads) : m_hazard_ptrs { max_threads, 1 }
{
	int log_size;
}

template <typename T>
void
deque<T>::push_bottom(deque::pointer elem, std::size_t thread_id)
{
	const auto b = this->m_bottom.load(std::memory_order::relaxed);
	const auto t = this->m_top.load(std::memory_order::acquire);

	auto active = this->m_active.load(std::memory_order::relaxed);
	const auto size = b - t;
	if (size >= active->size - 1) {
		auto old = active;
		active = active.grow(b, t);
		this->m_active.store(active, std::memory_order::relaxed);
		this->m_hazard_ptrs.retire(thread_id, old);
	}

	active.write(b, elem);
	this->m_bottom.store(b + 1, std::memory_order_release);
}

template <typename T>
deque<T>::pointer
deque<T>::pop_bottom(std::size_t thread_id)
{
	auto b = this->m_bottom.load(std::memory_order::relaxed) - 1;
	auto active = this->m_active.load(std::memory_order::relaxed);
	this->m_bottom.store(b, std::memory_order::relaxed);
	auto t = this->m_top.load(std::memory_order::acquire);

	const auto size = b - t;
	if (size < 0) {
		this->m_bottom.store(top, std::memory_order::relaxed);
		return nullptr;
	}

	auto elem = active.read(b);
	if (size > 0) {
		return elem;
	}

	if (!this->m_top.compare_exchange_strong(t, t + 1,
      std::memory_order::release, std::memory_order::relaxed
  ) {
		elem = nullptr;
  }

  this->m_bottom.store(t + 1, std::memory_order_relaxed);
  return elem;
}

template <typename T>
steal_res_t
deque<T>::steal(deque::pointer &elem, std::size_t thread_id)
{
	auto t = this->m_top.load(std::memory_order::acquire);
	auto b = this->m_top.load(std::memory_order::acquire);
	const auto active = this->m_hazard_ptrs.protect(this->m_active, thread_id, 0);
	const auto size = b - t;
	if (size <= 0) {
		return steal_res_t::Empty;
	}

	elem = active.read(t);
	auto res = steal_res_t::Success;
	if (!this->m_top.compare_exchange_strong(t, t + 1, std::memory_order::release,
				std::memory_order::relaxed)) {
		res = steal_res_t::Retry;
	}

	this->m_hazard_ptrs.clear(thread_id);
	return res;
}
} /* cld */

#endif /* LFQUEUE_BENCHMARKS_ */
