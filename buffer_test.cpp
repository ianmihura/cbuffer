#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <cassert>
#include <functional>
#include <x86intrin.h>
#include <sys/time.h>

#include "buffer.hpp"
#include "cbuffer.hpp"

struct bench_results
{
    double seconds;
    double metric; // currently measuring gbps thruput
};

bench_results bench(const size_t iter, std::function<void()> fn)
{
    double min_seconds = 10000000.0;
    struct timespec start, end;

    for (int i = 0; i < iter; ++i)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        fn();
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        double seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        if (seconds < min_seconds)
        {
            min_seconds = seconds;
        }
    }

    return (bench_results){ min_seconds };
}

void print_results(bench_results *results, double bytes)
{
    double bytes_per_sec = bytes / results->seconds;
    double gib_per_sec = bytes_per_sec / (1024.0 * 1024.0 * 1024.0);
    
    printf("    Time (s): %.9f\n", results->seconds);
    printf("    Throughput: %.3f GiB/s  (%.0f B/s)\n", gib_per_sec, bytes_per_sec);

    (*results).metric = gib_per_sec;
}

struct bench_results_bufs
{
    double cbuf_metric;
    double buf_metric;
};

// Writes sequentially through the buffer, element by element.
// 
// `iter` how many iterations
// `count` is length of buffers, how many elements to write per iteration.
bench_results_bufs bench_sequential_write(size_t count, size_t iter)
{
    Buffer<uint8_t> buf(count);
    CBuffer<uint8_t> cbuf(count * sizeof(uint8_t));

    printf("\nSequential write, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        for (size_t i = 0; i < count; ++i)
        {
            buf[i] = static_cast<uint8_t>(i);
        }
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        for (size_t i = 0; i < count; ++i)
        {
            cbuf[i] = static_cast<uint8_t>(i);
        }
    });
    double bytes = (double)sizeof(uint8_t) * count;

    printf("  Buffer best run:\n");
    print_results(&bench_results_buf, bytes);

    printf("  CBuffer best run:\n");
    print_results(&bench_results_cbuf, bytes);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

// Reads sequentially through the buffer, accumulating a checksum.
// 
// `iter` how many iterations
// `count` is length of buffers, how many elements to write per iteration.
bench_results_bufs bench_sequential_read(size_t count, size_t iter)
{
    Buffer<uint8_t> buf(count);
    CBuffer<uint8_t> cbuf(count * sizeof(uint8_t));

    // Pre-fill both buffers identically
    for (size_t i = 0; i < count; ++i)
    {
        buf[i] = static_cast<uint8_t>(i);
        cbuf[i] = static_cast<uint8_t>(i);
    }
    auto expected_sum = count*(count+1)/2 - count;

    printf("\nSequential read, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i)
        {
            sum += buf[i];
        }
        assert(expected_sum == sum);
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i)
        {
            sum += cbuf[i];
        }
        assert(expected_sum == sum);
    });
    double bytes = (double)sizeof(uint8_t) * count;

    printf("  Buffer best run:\n");
    print_results(&bench_results_buf, bytes);

    printf("  CBuffer best run:\n");
    print_results(&bench_results_cbuf, bytes);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

// Writes with a wraparound, element by element.
// 
// `iter` how many iterations
// `count` is length of buffers, how many elements to write per iteration.
bench_results_bufs bench_wraparound_write(size_t count, size_t iter)
{
    Buffer<uint8_t> buf(count);
    CBuffer<uint8_t> cbuf(count * sizeof(uint8_t));

    printf("\nWraparound write, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        for (size_t i = count; i < 2*count; ++i)
        {
            buf[i] = static_cast<uint8_t>(i);
        }
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        for (size_t i = count; i < 2*count; ++i)
        {
            cbuf[i] = static_cast<uint8_t>(i);
        }
    });
    double bytes = (double)sizeof(uint8_t) * count;

    printf("  Buffer best run:\n");
    print_results(&bench_results_buf, bytes);

    printf("  CBuffer best run:\n");
    print_results(&bench_results_cbuf, bytes);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

// Reads with a wraparound, accumulating a checksum.
// 
// `iter` how many iterations
// `count` is length of buffers, how many elements to write per iteration.
bench_results_bufs bench_wraparound_read(size_t count, size_t iter)
{
    Buffer<uint8_t> buf(count);
    CBuffer<uint8_t> cbuf(count * sizeof(uint8_t));

    // Pre-fill both buffers identically
    for (size_t i = 0; i < count; ++i)
    {
        buf[i] = static_cast<uint8_t>(i);
        cbuf[i] = static_cast<uint8_t>(i);
    }
    auto expected_sum = count*(count+1)/2 - count;

    printf("\nWraparound read, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = count; i < 2*count; ++i)
        {
            sum += buf[i];
        }
        assert(expected_sum == sum);
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = count; i < 2*count; ++i)
        {
            sum += cbuf[i];
        }
        assert(expected_sum == sum);
    });
    double bytes = (double)sizeof(uint8_t) * count;

    printf("  Buffer best run:\n");
    print_results(&bench_results_buf, bytes);

    printf("  CBuffer best run:\n");
    print_results(&bench_results_cbuf, bytes);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

int main()
{
    
    int loops = 6;
    size_t counts[6] = {4096, 16*4096, 128*4096, 1024*4096, 2048*4096, 4096*4096};
    size_t iters[6] = {100000, 10000, 1000, 1000, 100, 100};

    bench_results_bufs bench_results_metrics[4*loops];
    for (int i = 0; i < loops; ++i)
    {
        bench_results_metrics[0+4*i] = bench_sequential_write(counts[i], iters[i]);
        bench_results_metrics[1+4*i] = bench_sequential_read(counts[i], iters[i]);
        bench_results_metrics[2+4*i] = bench_wraparound_write(counts[i], iters[i]);
        bench_results_metrics[3+4*i] = bench_wraparound_read(counts[i], iters[i]);
    }

    for (int i = 0; i < 15; ++i) {
        printf("%lf,%lf\n", bench_results_metrics[i].buf_metric, bench_results_metrics[i].cbuf_metric);
    }

    return 0;
}
