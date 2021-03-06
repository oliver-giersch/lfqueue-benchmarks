#include <array>
#include <charconv>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

#include "boost/thread/barrier.hpp"

#include "common.hpp"
#include "queues/faa/faa_array.hpp"
#include "queues/lcr/lcrq.hpp"
#include "queues/msc/michael_scott.hpp"
#include "queues/lsc/lscq.hpp"
#include "queues/queue_ref.hpp"

#include "looqueue/queue.hpp"
#include "ymcqueue/queue.hpp"

constexpr std::array<std::size_t, 11> THREADS{ 1, 2, 4, 8, 16, 24, 32, 48, 64, 80, 96 };

using faa::detail::queue_variant_t;
using thread_span_t = std::span<const std::size_t>;

/********** queue aliases *****************************************************/

using loo_queue        = loo::queue<std::size_t>;

using faa_queue        = faa::queue<std::size_t>;
using faa_queue_ref    = faa::queue_ref<std::size_t>;
using faa_queue_v1     = faa::queue<std::size_t, queue_variant_t::VARIANT_1>;
using faa_queue_v1_ref = faa::queue_ref_v1<std::size_t>;
using faa_queue_v2     = faa::queue<std::size_t, queue_variant_t::VARIANT_2>;
using faa_queue_v2_ref = faa::queue_ref_v2<std::size_t>;
using faa_queue_v3     = faa::queue<std::size_t, queue_variant_t::VARIANT_3>;
using faa_queue_v3_ref = faa::queue_ref_v3<std::size_t>;
using lcr_queue        = lcr::queue<std::size_t>;
using lcr_queue_ref    = lcr::queue_ref<std::size_t>;
using lscqd_queue      = scq::d::queue<std::size_t>;
using lscqd_queue_ref  = scq::d::queue_ref<std::size_t>;
using lscq2_queue      = scq::cas2::queue<std::size_t>;
using lscq2_queue_ref  = scq::cas2::queue_ref<std::size_t>;
using msc_queue        = msc::queue<std::size_t>;
using msc_queue_ref    = msc::queue_ref<std::size_t>;
using ymc_queue        = ymc::queue<std::size_t>;
using ymc_queue_ref    = queue_ref<ymc_queue>;

/********** function pointer aliases ******************************************/

template <typename Q, typename R>
using make_queue_ref_fn = std::function<R(Q&, std::size_t)>;

/********** functions *********************************************************/

/** runs all bench iterations for the specified bench and queue */
template <typename Q, typename R>
void run_benches(
    std::string_view        queue_name,
    bench::bench_type_t     bench_type,
    std::size_t             total_ops,
    std::size_t             runs,
    thread_span_t           threads_range,
    make_queue_ref_fn<Q, R> make_queue_ref
);

/** runs the pairwise enqueue/dequeue benchmark */
template <typename Q, typename R>
void bench_pairwise(
    std::string_view        queue_name,
    std::size_t             total_ops,
    std::size_t             runs,
    std::size_t             threads,
    make_queue_ref_fn<Q, R> make_queue_ref
);

/** runs the burst benchmarks */
template <typename Q, typename R>
void bench_bursts(
    std::string_view        queue_name,
    std::size_t             total_ops,
    std::size_t             runs,
    std::size_t             threads,
    make_queue_ref_fn<Q, R> make_queue_ref
);

/** runs either the read-heavy or write-heavy benchmark */
template <typename Q, typename R>
void bench_reads_or_writes(
    std::string_view        queue_name,
    bench::bench_type_t     bench_type,
    std::size_t             total_ops,
    std::size_t             runs,
    std::size_t             threads,
    make_queue_ref_fn<Q, R> make_queue_ref
);

/** potentially extracts the alternative threads span from the argument vector */
thread_span_t extract_thread_span(
    int argc,
    char* argv[6],
    std::array<std::size_t, 1>& res
) {
  if (argc < 6) {
    return std::span(THREADS.begin(), THREADS.end());
  } else {
    const std::string_view str{ argv[5] };
    const auto err = std::from_chars(str.begin(), str.end(), res[0]);
    if (err.ec != std::errc()) {
      throw std::invalid_argument("alternative thread range: expected integer");
    }

    return std::span(res.begin(), res.end());
  }
}

int main(int argc, char* argv[5]) {
  if (argc < 5) {
    throw std::invalid_argument("too few program arguments");
  }

  const std::string_view queue{ argv[1] };
  const std::string_view bench{ argv[2] };
  const std::string_view total_ops_str{ argv[3] };
  const std::string_view runs_str{ argv[4] };

  const auto queue_type = bench::parse_queue_str(queue);
  const auto bench_type = bench::parse_bench_str(bench);
  const auto total_ops = bench::parse_total_ops_str(total_ops_str);
  const auto runs = bench::parse_runs_str(runs_str);

  auto alternative_thread_range = std::to_array({ static_cast<std::size_t>(0) });
  const auto threads = extract_thread_span(argc, argv, alternative_thread_range);

  const std::string_view queue_name{ bench::display_str(queue_type) };

  switch (queue_type) {
    case bench::queue_type_t::LCR:
      run_benches<lcr_queue, lcr_queue_ref>(
          queue_name, bench_type, total_ops, runs, threads,
          [](auto& queue, auto thread_id) -> auto {
            return lcr_queue_ref(queue, thread_id);
          }
      );
      break;
    case bench::queue_type_t::LOO:
      run_benches<loo_queue, loo_queue&>(
          queue_name, bench_type, total_ops, runs, threads,
          [](auto& queue, auto) -> auto& { return queue; }
      );
      break;
    case bench::queue_type_t::FAA:
      run_benches<faa_queue, faa_queue_ref>(
          queue_name, bench_type, total_ops, runs, threads,
          [](auto& queue, auto thread_id) -> auto {
            return faa_queue_ref(queue, thread_id);
          }
      );
      break;
    case bench::queue_type_t::FAA_V1:
      run_benches<faa_queue_v1, faa_queue_v1_ref>(
          queue_name, bench_type, total_ops, runs, threads,
          [](auto& queue, auto thread_id) -> auto {
            return faa_queue_v1_ref(queue, thread_id);
          }
      );
      break;
    case bench::queue_type_t::FAA_V2:
      run_benches<faa_queue_v2, faa_queue_v2_ref>(
          queue_name, bench_type, total_ops, runs, threads,
          [](auto& queue, auto thread_id) -> auto {
            return faa_queue_v2_ref(queue, thread_id);
          }
      );
      break;
      case bench::queue_type_t::FAA_V3:
        run_benches<faa_queue_v3, faa_queue_v3_ref>(
          queue_name, bench_type, total_ops, runs, threads,
          [](auto& queue, auto thread_id) -> auto {
            return faa_queue_v3_ref(queue, thread_id);
          }
        );
    break;
    case bench::queue_type_t::MSC:
      run_benches<msc_queue, msc_queue_ref>(
          queue_name, bench_type, total_ops, runs, threads,
          [](auto& queue, auto thread_id) -> auto {
            return msc_queue_ref(queue, thread_id);
          }
      );
      break;
    case bench::queue_type_t::SCQ2:
      run_benches<lscq2_queue, lscq2_queue_ref>(
          queue_name, bench_type, total_ops, runs, threads,
          [](auto& queue, auto thread_id) -> auto {
            return lscq2_queue_ref(queue, thread_id);
          }
      );
      break;
    case bench::queue_type_t::SCQD:
      run_benches<lscqd_queue, lscqd_queue_ref>(
          queue_name, bench_type, total_ops, runs, threads,
          [](auto& queue, auto thread_id) -> auto {
            return lscqd_queue_ref(queue, thread_id);
          }
      );
      break;
    case bench::queue_type_t::YMC:
      run_benches<ymc_queue, ymc_queue_ref>(
          queue_name, bench_type, total_ops, runs, threads,
          [](auto& queue, auto thread_id) -> auto {
            return ymc_queue_ref(queue, thread_id);
          }
      );
      break;
  }
}

template <typename Q, typename R>
void run_benches(
    std::string_view        queue_name,
    bench::bench_type_t     bench_type,
    std::size_t             total_ops,
    std::size_t             runs,
    thread_span_t           threads_range,
    make_queue_ref_fn<Q, R> make_queue_ref
) {
  if (bench_type == bench::bench_type_t::PAIRS || bench_type == bench::bench_type_t::BURSTS) {
    for (auto threads : threads_range) {
      // aborts if hyper-threads would be used (assuming 2 HT per core)
      if (threads > std::thread::hardware_concurrency() / 2) {
        break;
      }

      switch (bench_type) {
        case bench::bench_type_t::PAIRS:
          bench_pairwise<Q, R>(queue_name, total_ops, runs, threads, make_queue_ref);
          break;
        case bench::bench_type_t::BURSTS:
          bench_bursts<Q, R>(queue_name, total_ops, runs, threads, make_queue_ref);
          break;
        default: throw std::runtime_error("unreachable branch");
      }
    }
  } else {
    for (auto threads : threads_range) {
      if (threads < 4) {
        continue;
      }

      // aborts if hyper-threads would be used (assuming 2 HT per core)
      if (threads > std::thread::hardware_concurrency() / 2) {
        break;
      }

      bench_reads_or_writes<Q, R>(queue_name, bench_type, total_ops, runs, threads, make_queue_ref);
    }
  }
}

template <typename Q, typename R>
void bench_pairwise(
    std::string_view        queue_name,
    std::size_t             total_ops,
    std::size_t             runs,
    std::size_t             threads,
    make_queue_ref_fn<Q, R> make_queue_ref
) {
  const auto ops_per_threads = total_ops / threads;

  // pre-allocates a vector for storing the elements enqueued by each thread;
  std::vector<std::size_t> thread_ids{};
  thread_ids.reserve(threads);
  for (auto thread = 0; thread < threads; ++thread) {
    thread_ids.push_back(thread);
  }

  // execute benchmark for `runs` iterations
  for (auto run = 0; run < runs; ++run) {
    auto queue = std::make_unique<Q>();
    boost::barrier barrier{ static_cast<unsigned>(threads + 1) };

    // pre-allocates a vector for storing each thread's join handle
    std::vector<std::thread> thread_handles{};
    thread_handles.reserve(threads);

    // spawns threads and performs pairwise enqueue and dequeue operations
    for (auto thread = 0; thread < threads; ++thread) {
      thread_handles.emplace_back(std::thread([&, thread] {
        bench::pin_current_thread(thread);

        auto&& queue_ref = make_queue_ref(*queue, thread);

        // all threads synchronize at this barrier before starting
        barrier.wait();

        for (auto op = 0; op < ops_per_threads; ++op) {
          if (op % 2 == 0) {
            queue_ref.enqueue(&thread_ids.at(thread));
          } else {
            auto elem = queue_ref.dequeue();
            if (elem != nullptr && (elem < &thread_ids.front() || elem > &thread_ids.back())) {
              throw std::runtime_error(
                  "invalid element retrieved (undefined behaviour detected)"
              );
            }
          }
        }

        // all threads synchronize at this barrier before completing
        barrier.wait();
      }));
    }

    barrier.wait();
    // measures total time once all threads have arrived at the barrier
    const auto start = std::chrono::high_resolution_clock::now();
    barrier.wait();
    const auto stop = std::chrono::high_resolution_clock::now();
    const auto duration = stop - start;

    // joins all threads
    for (auto& handle : thread_handles) {
      handle.join();
    }

    // print measurements to stdout
    std::cout
        << queue_name
        << "," << threads
        << "," << duration.count()
        << "," << total_ops << std::endl;
  }
}

template <typename Q, typename R>
void bench_bursts(
    std::string_view        queue_name,
    std::size_t             total_ops,
    std::size_t             runs,
    std::size_t             threads,
    make_queue_ref_fn<Q, R> make_queue_ref
) {
  const auto ops_per_threads = total_ops / threads;

  // pre-allocates a vector for storing the elements enqueued by each thread;
  std::vector<std::size_t> thread_ids{};
  thread_ids.reserve(threads);
  for (auto thread = 0; thread < threads; ++thread) {
    thread_ids.push_back(thread);
  }

  // execute benchmark for `runs` iterations
  for (auto run = 0; run < runs; ++run) {
    auto queue = std::make_unique<Q>();
    boost::barrier barrier{ static_cast<unsigned>(threads + 1) };

    // pre-allocates a vector for storing each thread's join handle
    std::vector<std::thread> thread_handles{};
    thread_handles.reserve(threads);

    // spawns threads and performs pairwise enqueue and dequeue operations
    for (auto thread = 0; thread < threads; ++thread) {
      thread_handles.emplace_back(std::thread([&, thread] {
        bench::pin_current_thread(thread);

        auto&& queue_ref = make_queue_ref(*queue, thread);

        // (1) all threads synchronize at this barrier before starting
        barrier.wait();

        for (auto op = 0; op < ops_per_threads; ++op) {
          queue_ref.enqueue(&thread_ids.at(thread));
        }

        // (2) all threads synchronize at this barrier after completing their
        // respective enqueue burst
        barrier.wait();

        for (auto op = 0; op < ops_per_threads; ++op) {
          auto elem = queue_ref.dequeue();
          // can never be null in fact
          if (elem != nullptr && (elem < &thread_ids.front() || elem > &thread_ids.back())) {
            throw std::runtime_error(
                "invalid element retrieved (undefined behaviour detected)"
            );
          }
        }

        // (3) all threads synchronize at this barrier before completing
        barrier.wait();
      }));
    }

    // (1)
    barrier.wait();
    // measures total time of enqueue burst once all threads have arrived at the
    // barrier
    const auto enq_start = std::chrono::high_resolution_clock::now();
    // (2)
    barrier.wait();
    const auto enq_stop = std::chrono::high_resolution_clock::now();
    // (3)
    barrier.wait();
    const auto deq_stop = std::chrono::high_resolution_clock::now();

    const auto enq = enq_stop - enq_start;
    const auto deq = deq_stop - enq_stop;

    // joins all threads
    for (auto& handle : thread_handles) {
      handle.join();
    }

    // print measurements to stdout
    std::cout
        << queue_name
        << "," << threads
        << "," << enq.count()
        << "," << deq.count()
        << "," << total_ops << std::endl;
  }
}

template <typename Q, typename R>
void bench_reads_or_writes(
    std::string_view        queue_name,
    bench::bench_type_t     bench_type,
    std::size_t             total_ops,
    std::size_t             runs,
    std::size_t             threads,
    make_queue_ref_fn<Q, R> make_queue_ref
) {
  if (threads % 4 != 0) {
    throw std::invalid_argument("argument `threads` must be multiple of four");
  }

  const auto ops_per_thread = total_ops / threads;

  // pre-allocates a vector for storing the elements enqueued by each thread
  std::vector<std::size_t> thread_ids{};
  thread_ids.reserve(threads);
  for (auto thread = 0; thread < threads; ++thread) {
    thread_ids.push_back(thread);
  }

  // execute benchmark for `runs` iterations
  for (auto run = 0; run < runs; ++run) {
    auto queue = std::make_unique<Q>();
    boost::barrier barrier{ static_cast<unsigned>(threads + 1) };

    // pre-allocates a vector for storing each thread's join handle
    std::vector<std::thread> thread_handles{};
    thread_handles.reserve(threads);

    // for read-heavy benchmarks, seed the queue with large number of values
    // before starting the benchmark
    if (bench_type == bench::bench_type_t::READS) {
      auto&& queue_ref = make_queue_ref(*queue, 0);
      for (auto op = 0; op < (3 * total_ops) / 4; ++op) {
        queue_ref.enqueue(&thread_ids.at(0));
      }
    }

    // spawns threads and performs the required operations
    for (auto thread = 0; thread < threads; ++thread) {
      thread_handles.emplace_back(std::thread([&, thread] {
        bench::pin_current_thread(thread);

        auto&& queue_ref = make_queue_ref(*queue, thread);

        const auto writer_thread = [&]() {
          for (auto op = 0; op < ops_per_thread; ++op) {
            queue_ref.enqueue(&thread_ids.at(thread));
          }
        };

        const auto reader_thread = [&]() {
          for (auto op = 0; op < ops_per_thread; ++op) {
            auto elem = queue_ref.dequeue();
            if (elem == nullptr) {
              bench::spin_for_ns(50);
            } else {
              if (elem < &thread_ids.front() || elem > &thread_ids.back()) {
                throw std::runtime_error(
                    "invalid element retrieved (undefined behaviour detected)"
                );
              }
            }
          }
        };

        // all threads synchronize at this barrier before starting
        barrier.wait();

        switch (bench_type) {
          case bench::bench_type_t::WRITES:
            if (thread % 4 == 0) {
              reader_thread();
            } else {
              writer_thread();
            }
            break;
          case bench::bench_type_t::READS:
            if (thread % 4 == 0) {
              writer_thread();
            } else {
              reader_thread();
            }
            break;
          case bench::bench_type_t::MIXED:
            if (thread % 2 == 0) {
              writer_thread();
            } else {
              reader_thread();
            }
            break;
          default: throw std::runtime_error("unreachable branch");
        }

        // all threads synchronize at this barrier before finishing
        barrier.wait();
      }));
    }

    // synchronize with threads before starting
    barrier.wait();
    // measures total time once all threads have arrived at the barrier
    const auto start = std::chrono::high_resolution_clock::now();
    // synchronize with threads after finishing
    barrier.wait();
    const auto stop = std::chrono::high_resolution_clock::now();
    const auto duration = stop - start;

    // joins all threads
    for (auto& handle : thread_handles) {
      handle.join();
    }

    // print measurements to stdout
    std::cout
        << queue_name
        << "," << threads
        << "," << duration.count()
        << "," << total_ops << std::endl;
  }
}
