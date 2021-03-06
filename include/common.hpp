#ifndef LOO_QUEUE_BENCHES_COMMON_HPP
#define LOO_QUEUE_BENCHES_COMMON_HPP

#include <string_view>

namespace bench {
enum class bench_type_t { PAIRS, BURSTS, READS, WRITES, MIXED };
enum class queue_type_t { LCR, LOO, FAA, FAA_V1, FAA_V2, FAA_V3, MSC, SCQ2, SCQD, YMC };

constexpr std::string_view display_str(queue_type_t queue) {
  switch (queue) {
    case queue_type_t::LCR:    return "LCR";
    case queue_type_t::LOO:    return "LOO";
    case queue_type_t::FAA:    return "FAA";
    case queue_type_t::FAA_V1: return "FAA (variant 1)";
    case queue_type_t::FAA_V2: return "FAA (variant 2)";
    case queue_type_t::FAA_V3: return "FAA (variant 3)";
    case queue_type_t::MSC:    return "MSC";
    case queue_type_t::SCQ2:   return "LSCQ2";
    case queue_type_t::SCQD:   return "LSCQD";
    case queue_type_t::YMC:    return "YMC";
    default:                   return "unknown";
  }
}

/** parses the given string to the corresponding queue type */
queue_type_t parse_queue_str(std::string_view queue);
/** parses the given string to the corresponding bench type */
bench_type_t parse_bench_str(std::string_view bench);
/** parses the benchmark `size` argument string */
std::size_t  parse_total_ops_str(std::string_view total_ops);
/** parses the benchmark `runs` argument string */
std::size_t  parse_runs_str(std::string_view runs);
/** pins the thread with the given id to the core with the same number */
void pin_current_thread(std::size_t thread_id);
/** spins the current thread for at least `ns` nanoseconds */
void spin_for_ns(std::size_t ns);
}

#endif /* LOO_QUEUE_BENCHES_COMMON_HPP */
