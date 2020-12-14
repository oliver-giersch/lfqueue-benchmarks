#include <atomic>
#include <concepts>
#include <iostream>
#include <string>
#include <stdexcept>
#include <thread>
#include <vector>

#include "common.hpp"

#include "queues/faa/faa_array.hpp"
#include "queues/lcr/lcrq.hpp"
#include "queues/lsc/lscq.hpp"
#include "queues/msc/michael_scott.hpp"

#include "ymcqueue/queue.hpp"

constexpr std::size_t THREAD_COUNT = 8;
constexpr std::size_t COUNT = 100 * 1'000;

constexpr auto EXPECTED = THREAD_COUNT * (COUNT * (COUNT - 1) / 2);

template <typename Q, typename T>
concept ConcurrentQueue =
    requires(Q queue, T* elem, std::size_t thread_id)
{
  { queue.enqueue(elem, thread_id) } -> std::same_as<void>;
  { queue.dequeue(thread_id) } -> std::same_as<T*>;
};

template <ConcurrentQueue<std::size_t> Q>
bool test_queue(Q& queue);

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    throw std::runtime_error("no queue argument given");
  }

  const auto queue_variant = std::string{ argv[1] };

  switch (bench::parse_queue_str(queue_variant)) {
    case bench::queue_type_t::FAA: {
      faa::queue<std::size_t> queue{};
      return !test_queue(queue);
    }
    case bench::queue_type_t::LCR: {
      lcr::queue<std::size_t> queue{};
      return !test_queue(queue);
    }
    case bench::queue_type_t::LSC: {
      lsc::queue<std::size_t> queue{};
      return !test_queue(queue);
    }
    case bench::queue_type_t::MSC: {
      msc::queue<std::size_t> queue{};
      return !test_queue(queue);
    }
    case bench::queue_type_t::YMC: {
      ymc::queue<std::size_t> queue{};
      return !test_queue(queue);
    }
    default: throw std::runtime_error("unsupported queue variant");
  }
}

template <ConcurrentQueue<std::size_t> Q>
bool test_queue(Q& queue) {
  std::vector<std::vector<std::size_t>> thread_elements{ };
  thread_elements.reserve(THREAD_COUNT);

  for (auto thread = 0; thread < THREAD_COUNT; ++thread) {
    thread_elements.emplace_back();
    thread_elements.back().reserve(COUNT);

    for (auto i = 0; i < COUNT; ++i) {
      thread_elements.back().push_back(i);
    }
  }

  std::vector<std::thread> threads{};
  threads.reserve(THREAD_COUNT * 2);

  std::atomic_bool start{ false };
  std::atomic_uint64_t sum{ 0 };

  for (auto thread = 0; thread < THREAD_COUNT; ++thread) {
    // producer thread
    threads.emplace_back([&, thread] {
      while (!start.load()) {}

      for (auto op = 0; op < COUNT; ++op) {
        queue.enqueue(&thread_elements.at(thread).at(op), thread);
      }
    });

    // consumer thread
    const auto deq_id = thread + THREAD_COUNT;
    threads.emplace_back([&, deq_id] {
      uint64_t thread_sum = 0;
      uint64_t deq_count  = 0;

      while (!start.load()) {}

      while (deq_count < COUNT) {
        const auto res = queue.dequeue(deq_id);
        if (res != nullptr) {
          if (*res >= COUNT) {
            throw std::runtime_error("invalid element dequeued");
          }

          thread_sum += *res;
          deq_count += 1;
        }
      }

      sum.fetch_add(thread_sum);
    });
  }

  start.store(true);

  for (auto& thread : threads) {
    thread.join();
  }

  if (queue.dequeue(0) != nullptr) {
    std::cerr << "queue not empty after count * threads dequeue operations" << std::endl;
    return false;
  }

  const auto res = sum.load();
  if (res != EXPECTED) {
    std::cerr << "incorrect element sum, got " << sum << ", expected " << EXPECTED << std::endl;
    return false;
  }

  std::cout << "test successful" << std::endl;
  return true;
}
