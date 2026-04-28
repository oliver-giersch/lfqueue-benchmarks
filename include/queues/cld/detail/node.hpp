#ifndef LFQUEUE_BENCHMARKS_CHASE_LEV_NODE_HPP
#define LFQUEUE_BENCHMARKS_CHASE_LEV_NODE_HPP

#include <atomic>
#include <utility>

namespace cld {
template <typename T>
struct deque<T>::node_t {
	using pointer = T *;

	int log_size;
	std::size_t size;
	pointer elems[];

	static node_t *
	alloc(int log_size)
	{
		auto size = 1 << log_size;
		char *mem = new char[sizeof(node_t) + (size * sizeof(pointer))];
		new (mem) node_t(log_size, size);

		return static_cast<node_t *>(mem);
	}

	void
	operator delete(void *node)
	{
		auto mem = static_cast<char *>(node);
		delete[] mem;
	}

	explicit node_t(int log_size, std::size_t size)
	{
		this->log_size = log_size;
		this->size = size;

		for (auto i = 0; i < size; i++) {
			this->elems[i] = nullptr;
		}
	}

	node_t *
	grow(std::int64_t b, std::int64_t t)
	{
		auto next = node_t::alloc(this->log_size + 1);
		for (auto i = t; i < b; i++) {
			next->write(i, this->read(i));
		}

		return next;
	}

	pointer
	read(std::int64_t i)
	{
		return this->elems[i % this->size];
	}

	void
	write(std::int64_t i, pointer elem)
	{
		this->elems[i % this->size] = elem;
	}
};
} /* cld */

#endif /* LFQUEUE_BENCHMARKS_CHASE_LEV_NODE_HPP */