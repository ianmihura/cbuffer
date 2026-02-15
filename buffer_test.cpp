#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <functional>

#include "buffer.hpp"
#include "cbuffer.hpp"

// ── Timing helper ──────────────────────────────────────────────────
// Runs `fn` for `iterations` and returns elapsed nanoseconds.
static double bench(size_t iterations, std::function<void()> fn)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i)
        fn();
    auto t1 = std::chrono::high_resolution_clock::now();

    double ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    return ns;
}

// ── Reporting ──────────────────────────────────────────────────────
static void print_header(const size_t count, const size_t iter)
{
    printf("\n╔═════════════════════════════════════════════════════════════\n");
    printf("║ WORKLOAD:  buffer of %ld items (%ld bytes) iters=%ld\n", count, count*sizeof(int32_t), iter);
    printf("╠═════════════════════════════════════════════════════════════\n");
    printf("║  %-22s │ %12s │ %12s    \n", "Test", "Buffer (ns)", "CBuffer (ns)");
    printf("╠═════════════════════════════════════════════════════════════\n");
}

static void print_row(const char *test, double buf_ns, double cbuf_ns)
{
    const char *winner = (buf_ns <= cbuf_ns) ? "Buffer" : "CBuffer";
    double ratio = (buf_ns <= cbuf_ns) ? cbuf_ns / buf_ns : buf_ns / cbuf_ns;
    printf("║  %-22s │ %12.0f │ %12.0f    \n", test, buf_ns, cbuf_ns);
    printf("║  %22s │   winner: %-8s (%.2fx)     \n", "", winner, ratio);
}

static void print_footer()
{
    printf("╚═════════════════════════════════════════════════════════════\n");
}

// ── Sequential write benchmark ─────────────────────────────────────
// Writes sequentially through the buffer, element by element.
// `count` is how many int32 elements to write per iteration.
static void bench_sequential_write(size_t count, size_t iterations)
{
    Buffer<int32_t> buf(count);
    CBuffer<int32_t> cbuf(0, count * sizeof(int32_t));

    // The cbuffer virtual size might be larger after rounding,
    // so we only write `count` elements to keep it fair.
    volatile int32_t sink = 0; // prevent dead-code elimination

    double buf_ns = bench(iterations, [&]()
    {
        for (size_t i = 0; i < count; ++i)
            buf[i] = static_cast<int32_t>(i);
        sink = buf[count - 1];
    });

    double cbuf_ns = bench(iterations, [&]()
    {
        for (size_t i = 0; i < count; ++i)
            cbuf[i] = static_cast<int32_t>(i);
        sink = cbuf[count - 1];
    });

    (void)sink;
    print_row("Seq write", buf_ns, cbuf_ns);
}

// ── Sequential read benchmark ──────────────────────────────────────
// Reads sequentially through the buffer, accumulating a checksum.
static void bench_sequential_read(size_t count, size_t iterations)
{
    Buffer<int32_t> buf(count);
    CBuffer<int32_t> cbuf(count * sizeof(int32_t));

    // Pre-fill both buffers identically
    for (size_t i = 0; i < count; ++i)
    {
        buf[i] = static_cast<int32_t>(i);
        cbuf[i] = static_cast<int32_t>(i);
    }

    volatile int64_t sink = 0;

    double buf_ns = bench(iterations, [&]()
    {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i)
            sum += buf[i];
        sink = sum;
    });

    double cbuf_ns = bench(iterations, [&]()
    {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i)
            sum += cbuf[i];
        sink = sum;
    });

    (void)sink;
    print_row("Seq read", buf_ns, cbuf_ns);
}

// ── Wrap-around write benchmark ────────────────────────────────────
// Simulates a circular write pattern where the write index wraps.
// Buffer uses modulo; CBuffer relies on its virtual aliasing.
static void bench_wraparound_write(size_t phys_count, size_t total_writes, size_t iterations)
{
    Buffer<int32_t> buf(phys_count);
    // CBuffer with vsize = 2 * physical, so it mirrors once
    CBuffer<int32_t> cbuf(phys_count * sizeof(int32_t) * 2,
                          phys_count * sizeof(int32_t));

    volatile int32_t sink = 0;

    double buf_ns = bench(iterations, [&]()
    {
        for (size_t i = 0; i < total_writes; ++i)
            buf[i % phys_count] = static_cast<int32_t>(i);
        sink = buf[0];
    });

    // CBuffer: writes go to index i directly; the mmap aliasing
    // ensures indices beyond phys_count still land on physical memory.
    size_t cbuf_item_count = cbuf.GetVItemCount();
    double cbuf_ns = bench(iterations, [&]()
    {
        for (size_t i = 0; i < total_writes; ++i)
            cbuf[i % cbuf_item_count] = static_cast<int32_t>(i);
        sink = cbuf[0];
    });

    (void)sink;
    print_row("Wrap write", buf_ns, cbuf_ns);
}

static void run_bench(const size_t COUNT, const size_t ITERS)
{
    print_header(COUNT, ITERS);
    bench_sequential_write(COUNT, ITERS);
    bench_sequential_read(COUNT, ITERS);
    bench_wraparound_write(COUNT, COUNT * 4, ITERS);
    print_footer();
}

int main()
{
    printf("──────────────────────────────────────────────────────────────────\n");
    printf("  Buffer vs CBuffer — Performance Benchmark\n");
    printf("  Buffer  : malloc-backed flat array\n");
    printf("  CBuffer : mmap circular (virtual aliasing)\n");
    printf("──────────────────────────────────────────────────────────────────\n");

    // CBuffer against Buffer
    // run_bench(4096, 1000*1000);
    // run_bench(1024*1024, 1000);
    // run_bench(32*1024*1024, 50);

    return 0;
}
