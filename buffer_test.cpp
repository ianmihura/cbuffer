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
    uint64_t min_cycles;
    uint64_t avg_cycles;
};

bench_results bench(const size_t iter, std::function<void()> fn)
{
    uint64_t min_cycles = UINT64_MAX;
    uint64_t count_cycles = 0;

    for (int i = 0; i < iter; ++i)
    {
        std::chrono::high_resolution_clock::now();
        auto t0 = __rdtsc();
        fn();
        auto t1 = __rdtsc();

        uint64_t cycles = (t1 - t0);
        if (cycles < min_cycles)
        {
            min_cycles = cycles;
        }
        count_cycles += cycles;
    }

    return (bench_results){ min_cycles, count_cycles/iter };
}

void print_results(bench_results results, double bytes)
{
    const double cpu_hz = 2.3e9; // My cpu speed
    
    double seconds_cbuf = (double)results.min_cycles / cpu_hz;
    double bytes_per_sec_cbuf = bytes / seconds_cbuf;
    double gib_per_sec_cbuf = bytes_per_sec_cbuf / (1024.0 * 1024.0 * 1024.0);
    double items_per_cycle_cbuf = bytes / (double)results.min_cycles;
    
    printf("    Best cycle:\n");
    printf("    Time (s): %.9f\n", seconds_cbuf);
    printf("    Throughput: %.3f GiB/s  (%.0f B/s)\n", gib_per_sec_cbuf, bytes_per_sec_cbuf);
    printf("    Items per cycle: %.6f\n", items_per_cycle_cbuf);
    printf("    Avg cycle:\n");
    printf("    Time (s): %.9f\n", (double)results.avg_cycles / cpu_hz);
}

// Writes sequentially through the buffer, element by element.
// 
// `iter` how many iterations
// `count` is length of buffers, how many elements to write per iteration.
static void bench_sequential_write(size_t count, size_t iter)
{
    Buffer<int32_t> buf(count);
    CBuffer<int32_t> cbuf(0, count * sizeof(int32_t));

    printf("\nStarting test: sequential write\n");
    bench_results bench_results_buf = bench(iter, [&]() {
        for (size_t i = 0; i < count; ++i)
        {
            buf[i] = static_cast<int32_t>(i);
        }
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        for (size_t i = 0; i < count; ++i)
        {
            cbuf[i] = static_cast<int32_t>(i);
        }
    });
    printf("Done:\n");
    double bytes = (double)sizeof(int32_t) * count;

    printf("  Buffer:\n");
    print_results(bench_results_buf, bytes);

    printf("  CBuffer:\n");
    print_results(bench_results_cbuf, bytes);
}

// Reads sequentially through the buffer, accumulating a checksum.
// 
// `iter` how many iterations
// `count` is length of buffers, how many elements to write per iteration.
static void bench_sequential_read(size_t count, size_t iter)
{
    Buffer<int32_t> buf(count);
    CBuffer<int32_t> cbuf(0, count * sizeof(int32_t));

    // Pre-fill both buffers identically
    for (size_t i = 0; i < count; ++i)
    {
        buf[i] = static_cast<int32_t>(i);
        cbuf[i] = static_cast<int32_t>(i);
    }
    auto expected_sum = count*(count+1)/2 - count;

    printf("\nStarting test: sequential read\n");
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
    printf("Done:\n");
    double bytes = (double)sizeof(int32_t) * count;

    printf("  Buffer:\n");
    print_results(bench_results_buf, bytes);

    printf("  CBuffer:\n");
    print_results(bench_results_cbuf, bytes);
}

// Simulates a circular write where the write index wraps.
// Buffer uses modulo; CBuffer relies on its virtual aliasing.
// 
// `iter` how many iterations
// `count` is length of buffers, how many elements to write per iteration.
static void bench_wraparound_write(size_t count, size_t iter)
{
    Buffer<int32_t> buf(count);
    CBuffer<int32_t> cbuf(count * sizeof(int32_t)*2, count * sizeof(int32_t));

    // Pre-fill both buffers identically
    for (size_t i = 0; i < count; ++i)
    {
        buf[i] = static_cast<int32_t>(i);
        cbuf[i] = static_cast<int32_t>(i);
    }
    auto expected_sum = count*(count+1)/2 - count;

    printf("\nStarting test: sequential read\n");
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
    printf("Done:\n");
    double bytes = (double)sizeof(int32_t) * count;

    printf("  Buffer:\n");
    print_results(bench_results_buf, bytes);

    printf("  CBuffer:\n");
    print_results(bench_results_cbuf, bytes);
}

int main()
{
    // const int _1kb = 1024;
    // const int _4kb = 4096;
    // unsigned long _1gb = _1kb * _1kb * _1kb;

    // Buffer<uint8_t> buf(_4kb);                     // 4096 elements of size 1
    // CBuffer<uint8_t> cbuf(_4kb * sizeof(uint8_t)); // 4096 physical size

    // Fill(&buf);  // in linux we need to touch every page to initiate it
    // Fill(&cbuf); // in linux we need to touch every page to initiate it

    size_t COUNT = 10*4096;
    size_t ITERS = 10000;

    bench_sequential_write(COUNT, ITERS);
    bench_sequential_read(COUNT, ITERS);
    bench_wraparound_write(COUNT, ITERS);

    return 0;
}
