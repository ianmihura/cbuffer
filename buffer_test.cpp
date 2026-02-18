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

// so compiler doesnt optimize away
#define KEEP_ALIVE(val) asm volatile("" : : "g"(val) : "memory")

struct bench_results
{
    double seconds;
    double metric; // currently measuring gbps thruput
};

template <typename F>
bench_results bench(const size_t iter, F fn)
{
    double min_seconds = 10000000.0; // init to high number
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
    Buffer<uint32_t> buf(count);
    CBuffer<uint32_t> cbuf(count * sizeof(uint32_t));

    printf("\nSequential write, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        for (size_t i = 0; i < count; ++i)
        {
            buf[i] = static_cast<uint32_t>(i);
        }
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        for (size_t i = 0; i < count; ++i)
        {
            cbuf[i] = static_cast<uint32_t>(i);
        }
    });
    double bytes = (double)sizeof(uint32_t) * count;

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
    Buffer<uint32_t> buf(count);
    CBuffer<uint32_t> cbuf(count * sizeof(uint32_t));

    // Pre-fill both buffers identically
    for (size_t i = 0; i < count; ++i)
    {
        buf[i] = static_cast<uint32_t>(i);
        cbuf[i] = static_cast<uint32_t>(i);
    }
    size_t expected_sum = count*(count+1)/2 - count;
    // auto expected_sum = count*(UINT8_MAX)/2;

    printf("\nSequential read, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        int64_t sum = 0;
        KEEP_ALIVE(sum);
        for (size_t i = 0; i < count; ++i)
        {
            sum += buf[i];
        }
        // printf("%ld,%ld\n",expected_sum,sum);
        assert(expected_sum == sum);
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        int64_t sum = 0;
        KEEP_ALIVE(sum);
        for (size_t i = 0; i < count; ++i)
        {
            sum += cbuf[i];
        }
        assert(expected_sum == sum);
    });
    double bytes = (double)sizeof(uint32_t) * count;

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
    Buffer<uint32_t> buf(count);
    CBuffer<uint32_t> cbuf(count * sizeof(uint32_t));

    printf("\nWraparound write, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        for (size_t i = count; i < 2*count; ++i)
        {
            buf[i] = static_cast<uint32_t>(i);
        }
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        for (size_t i = count; i < 2*count; ++i)
        {
            cbuf[i] = static_cast<uint32_t>(i);
        }
    });
    double bytes = (double)sizeof(uint32_t) * count;

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
    Buffer<uint32_t> buf(count);
    CBuffer<uint32_t> cbuf(count * sizeof(uint32_t));

    // Pre-fill both buffers identically
    for (size_t i = 0; i < count; ++i)
    {
        buf[i] = static_cast<uint32_t>(i);
        cbuf[i] = static_cast<uint32_t>(i);
    }
    size_t expected_sum = count*(count+1)/2 - count;
    // auto expected_sum = count*(UINT8_MAX)/2;

    printf("\nWraparound read, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        int64_t sum = 0;
        KEEP_ALIVE(sum);
        for (size_t i = count; i < 2*count; ++i)
        {
            sum += buf[i];
        }
        assert(expected_sum == sum);
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        int64_t sum = 0;
        KEEP_ALIVE(sum);
        for (size_t i = count; i < 2*count; ++i)
        {
            sum += cbuf[i];
        }
        assert(expected_sum == sum);
    });
    double bytes = (double)sizeof(uint32_t) * count;

    printf("  Buffer best run:\n");
    print_results(&bench_results_buf, bytes);

    printf("  CBuffer best run:\n");
    print_results(&bench_results_cbuf, bytes);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

int main()
{
    int i;
    int loops = 6;
    size_t counts[6] = {4096, 16*4096, 128*4096, 1024*4096, 2048*4096, 4096*4096};
    size_t iters[6] = {100000, 10000, 1000, 1000, 100, 100};

    bench_results_bufs bench_results_metrics[4*loops];
    for (i = 0; i < loops; ++i)
    {
        bench_results_metrics[0+4*i] = bench_sequential_write(counts[i], iters[i]);
        bench_results_metrics[1+4*i] = bench_sequential_read(counts[i], iters[i]);
        bench_results_metrics[2+4*i] = bench_wraparound_write(counts[i], iters[i]);
        bench_results_metrics[3+4*i] = bench_wraparound_read(counts[i], iters[i]);
    }

    printf("count,buf_seq_w,cbuf_seq_w,buf_seq_r,cbuf_seq_r,buf_wrap_w,cbuf_wrap_w,buf_wrap_r,cbuf_wrap_r,\n");
    for (i = 0; i < loops; ++i) {
        printf("%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,\n",counts[i],
            bench_results_metrics[0+4*i].buf_metric, bench_results_metrics[0+4*i].cbuf_metric,
            bench_results_metrics[1+4*i].buf_metric, bench_results_metrics[1+4*i].cbuf_metric,
            bench_results_metrics[2+4*i].buf_metric, bench_results_metrics[2+4*i].cbuf_metric,
            bench_results_metrics[3+4*i].buf_metric, bench_results_metrics[3+4*i].cbuf_metric
        );
    }

    return 0;
}
