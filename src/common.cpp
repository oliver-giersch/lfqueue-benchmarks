#include "common.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>

#include <pthread.h>

namespace bench {
namespace {
std::size_t measure_ns_per_iteration() {
  using nanosecs = std::chrono::nanoseconds;
  constexpr auto iters = 1000;

  const auto start = std::chrono::high_resolution_clock::now();
  for (volatile auto i = 0; i < iters; ++i) {}
  const auto end = std::chrono::high_resolution_clock::now();

  const nanosecs dur = end - start;
  const auto ns_per_iter = dur.count() / iters;

  return ns_per_iter == 0 ? 1 : ns_per_iter;
}
}

queue_type_t parse_queue_str(const std::string& queue) {
  if (queue == "lcr") {
    return queue_type_t::LCR;
  }

  if (queue == "loo") {
    return queue_type_t::LOO;
  }

  if (queue == "faa") {
    return queue_type_t::FAA;
  }

  if (queue == "faa+") {
    return queue_type_t::FAA_PLUS;
  }

  if (queue == "msc") {
    return queue_type_t::MSC;
  }

  throw std::invalid_argument(
      "argument `queue` must be 'lcr', 'loo', 'faa', 'faa+' or 'msc'"
  );
}

bench_type_t parse_bench_str(const std::string& bench) {
  if (bench == "pairs") {
    return bench_type_t::PAIRS;
  }

  if (bench == "bursts") {
    return bench_type_t::BURSTS;
  }

  if (bench == "reads") {
    return bench_type_t::READS;
  }

  if (bench == "writes") {
    return bench_type_t::WRITES;
  }

  throw std::invalid_argument(
      "argument `bench` must be 'pairs', 'bursts', 'reads' or 'writes'"
  );
}

std::size_t parse_total_ops_str(const std::string& total_ops) {
  constexpr const char* ERR_MSG =
      "argument 'total_ops' must contain an integer number between 1 and 100 "
      "followed by either K or M";

  if (total_ops.size() <= 1) {
    throw std::invalid_argument(ERR_MSG);
  }

  const auto sub = total_ops.substr(0, total_ops.size() - 1);
  const auto fac = total_ops.back();

  auto val = std::stoi(sub);
  if (val <= 0 || val > 100) {
    throw std::invalid_argument(ERR_MSG);
  }

  switch (fac) {
    case 'K': return static_cast<std::size_t>(val) * 1024;
    case 'M': return static_cast<std::size_t>(val) * 1024 * 1024;
    default: throw std::invalid_argument(ERR_MSG);
  }
}

std::size_t parse_runs_str(const std::string& runs) {
  auto val = std::stoi(runs);
  if (val < 0 || val > 50) {
    throw std::invalid_argument("argument `runs` must be between 0 and 50");
  }

  return static_cast<std::size_t>(val);
}

void pin_current_thread(std::size_t thread_id) {
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(thread_id, &set);

  const auto res = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &set);
  if (res != 0) {
    throw std::system_error(res, std::generic_category(), "failed to pin thread");
  }
}

void spin_for_ns(std::size_t ns) {
  static const auto NS_PER_ITER = measure_ns_per_iteration();

  for (volatile auto i = 0; i < ns * NS_PER_ITER; ++i) {}
}
}
