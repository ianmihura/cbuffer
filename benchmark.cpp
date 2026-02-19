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

#define KA 1
#if KA
// so compiler doesnt optimize away
#define KEEP_ALIVE(val) asm volatile("" : : "g"(val) : "memory")
#else
#define KEEP_ALIVE(val) // pass
#endif

struct bench_results
{
    double seconds;
    double metric; // currently measuring gbps thruput
};

template <typename F, typename G>
bench_results bench(const size_t iter, F fn, G pre_fn)
{
    double min_seconds = 10000000.0; // init to high number
    struct timespec start, end;

    for (int i = 0; i < iter; ++i)
    {
        pre_fn();
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

// prints throughput to stdout, and writes perf metric (throughput) to result struct
void clean_results(bench_results *results, double bytes)
{
    double bytes_per_sec = bytes / results->seconds;
    double gib_per_sec = bytes_per_sec / (1024.0 * 1024.0 * 1024.0);
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
// `count` is length of buffers (items)
bench_results_bufs bench_sequential_write_typed(Buffer<uint32_t>* buf, CBuffer<uint32_t>* cbuf, size_t count, size_t iter)
{
    printf("\nSequential write, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        for (size_t i = 0; i < count; ++i)
        {
            (*buf)[i] = static_cast<uint32_t>(i);
        }
        KEEP_ALIVE(buf->Data);
    }, [&](){});

    bench_results bench_results_cbuf = bench(iter, [&]() {
        for (size_t i = 0; i < count; ++i)
        {
            (*cbuf)[i] = static_cast<uint32_t>(i);
        }
        KEEP_ALIVE(cbuf->Data);
    }, [&](){});
    double bytes = (double)sizeof(uint32_t) * count;

    printf("  Buffer best run:\n");
    clean_results(&bench_results_buf, bytes);

    printf("  CBuffer best run:\n");
    clean_results(&bench_results_cbuf, bytes);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

// Reads sequentially through the buffer, accumulating a checksum.
// 
// `iter` how many iterations
// `count` is length of buffers (items)
bench_results_bufs bench_sequential_read_typed(Buffer<uint32_t>* buf, CBuffer<uint32_t>* cbuf, size_t count, size_t iter)
{
    // Pre-fill both buffers identically
    for (size_t i = 0; i < count; ++i)
    {
        (*buf)[i] = static_cast<uint32_t>(i);
        (*cbuf)[i] = static_cast<uint32_t>(i);
    }
    size_t expected_sum = count*(count+1)/2 - count;
    // auto expected_sum = count*(UINT8_MAX)/2;

    printf("\nSequential read, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i)
        {
            sum += (*buf)[i];
        }
        KEEP_ALIVE(sum);
        assert(expected_sum == sum);
    }, [&](){});

    bench_results bench_results_cbuf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = 0; i < count; ++i)
        {
            sum += (*cbuf)[i];
        }
        KEEP_ALIVE(sum);
        assert(expected_sum == sum);
    }, [&](){});
    double bytes = (double)sizeof(uint32_t) * count;

    printf("  Buffer best run:\n");
    clean_results(&bench_results_buf, bytes);

    printf("  CBuffer best run:\n");
    clean_results(&bench_results_cbuf, bytes);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

// Writes with a wraparound, element by element.
// 
// `iter` how many iterations
// `count` is length of buffers (items)
bench_results_bufs bench_wraparound_write_typed(Buffer<uint32_t>* buf, CBuffer<uint32_t>* cbuf, size_t count, size_t iter)
{
    printf("\nWraparound write, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        for (size_t i = count; i < 2*count; ++i)
        {
            (*buf)[i] = static_cast<uint32_t>(i);
        }
        KEEP_ALIVE(buf->Data);
    }, [&](){});

    bench_results bench_results_cbuf = bench(iter, [&]() {
        for (size_t i = count; i < 2*count; ++i)
        {
            (*cbuf)[i] = static_cast<uint32_t>(i);
        }
        KEEP_ALIVE(cbuf->Data);
    }, [&](){});
    double bytes = (double)sizeof(uint32_t) * count;

    printf("  Buffer best run:\n");
    clean_results(&bench_results_buf, bytes);

    printf("  CBuffer best run:\n");
    clean_results(&bench_results_cbuf, bytes);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

// Reads with a wraparound, accumulating a checksum.
// 
// `iter` how many iterations
// `count` is length of buffers (items)
bench_results_bufs bench_wraparound_read_typed(Buffer<uint32_t>* buf, CBuffer<uint32_t>* cbuf, size_t count, size_t iter)
{
    // Pre-fill both buffers identically
    for (size_t i = 0; i < count; ++i)
    {
        (*buf)[i] = static_cast<uint32_t>(i);
        (*cbuf)[i] = static_cast<uint32_t>(i);
    }
    size_t expected_sum = count*(count+1)/2 - count;
    // auto expected_sum = count*(UINT8_MAX)/2;

    printf("\nWraparound read, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = count; i < 2*count; ++i)
        {
            sum += (*buf)[i];
        }
        KEEP_ALIVE(sum);
        assert(expected_sum == sum);
    }, [&](){});

    bench_results bench_results_cbuf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = count; i < 2*count; ++i)
        {
            sum += (*cbuf)[i];
        }
        KEEP_ALIVE(sum);
        assert(expected_sum == sum);
    }, [&](){});
    double bytes = (double)sizeof(uint32_t) * count;

    printf("  Buffer best run:\n");
    clean_results(&bench_results_buf, bytes);

    printf("  CBuffer best run:\n");
    clean_results(&bench_results_cbuf, bytes);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

void typed_buffer_benchmark() {
    int i;
    int loops = 7; //   4k    64k      512k      4m         8m         16m        256m
    size_t counts[7] = {4096, 16*4096, 128*4096, 1024*4096, 2048*4096, 4096*4096, 16*4096*4096};
    size_t iters[7] = {100000, 10000,  1000,     1000,      500,       500,       100};

    bench_results_bufs bench_results_metrics[4*loops];
    for (i = 0; i < loops; ++i)
    {
        Buffer<uint32_t> buf(counts[i]);
        CBuffer<uint32_t> cbuf(counts[i] * sizeof(uint32_t));
        assert(buf.Count == cbuf.GetPItemCount());
        bench_results_metrics[0+4*i] = bench_sequential_write_typed(&buf, &cbuf, counts[i], iters[i]);
        bench_results_metrics[1+4*i] = bench_sequential_read_typed(&buf, &cbuf, counts[i], iters[i]);
        bench_results_metrics[2+4*i] = bench_wraparound_write_typed(&buf, &cbuf, counts[i], iters[i]);
        bench_results_metrics[3+4*i] = bench_wraparound_read_typed(&buf, &cbuf, counts[i], iters[i]);
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
}

struct SomeData
{ // 32 bytes (2**5)
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    double e;
    double f;

    bool operator==(const SomeData &) const;
};
bool SomeData::operator==(const SomeData &other) const
{
    return (
        other.a == a &&
        other.b == b &&
        other.c == c &&
        other.e == e &&
        other.f == f
    );    
}

SomeData tmp_ = {11209976,0,1414,45,-53153.215,187.1025};

// Writes sequentially through the buffer, element by element.
// 
// `iter` how many iterations
// `count` is length of buffers (bytes)
bench_results_bufs bench_sequential_write_byte(ByteBuffer* buf, CByteBuffer* cbuf, size_t count, size_t iter)
{
    printf("\nSequential write, buffer size: %ld\n", count);
    size_t items = count/sizeof(SomeData);

    bench_results bench_results_buf = bench(iter, [&]() {
        for (size_t i = 0; i < items; ++i)
        {
            buf->Push(tmp_);
        }
        KEEP_ALIVE(buf->Data);
    }, [&](){ buf->Reset(); });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        for (size_t i = 0; i < items; ++i)
        {
            cbuf->Push(tmp_);
        }
        KEEP_ALIVE(cbuf->Data);
    }, [&](){ cbuf->Reset(); });

    printf("  Buffer best run:\n");
    clean_results(&bench_results_buf, count);

    printf("  CBuffer best run:\n");
    clean_results(&bench_results_cbuf, count);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

// Reads sequentially through the buffer, accumulating a checksum.
// 
// `iter` how many iterations
// `count` is length of buffers (bytes)
bench_results_bufs bench_sequential_read_byte(ByteBuffer* buf, CByteBuffer* cbuf, size_t count, size_t iter)
{
    // Both buffers are already prefilled
    size_t items = count/sizeof(SomeData);
    size_t expected_sum = items * tmp_.d;

    printf("\nSequential read, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = 0; i < items; ++i)
        {
            sum += buf->Pop<SomeData>().d;
        }
        KEEP_ALIVE(sum);
        assert(expected_sum == sum);
    }, [&](){
        buf->Reset();
        for (size_t i = 0; i < items; ++i)
        {
            buf->Push(tmp_);
        }
        KEEP_ALIVE(cbuf->Data);
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = 0; i < items; ++i)
        {
            sum += cbuf->Pop<SomeData>().d;
        }
        KEEP_ALIVE(sum);
        assert(expected_sum == sum);
    }, [&](){
        buf->Reset();
        for (size_t i = 0; i < items; ++i)
        {
            cbuf->Push(tmp_);
        }
        KEEP_ALIVE(cbuf->Data);
    });

    printf("  Buffer best run:\n");
    clean_results(&bench_results_buf, count);

    printf("  CBuffer best run:\n");
    clean_results(&bench_results_cbuf, count);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

// Writes with a wraparound, element by element.
// 
// `iter` how many iterations
// `count` is length of buffers (bytes)
bench_results_bufs bench_wraparound_write_byte(ByteBuffer* buf, CByteBuffer* cbuf, size_t count, size_t iter)
{
    printf("\nWraparound write, buffer size: %ld\n", count);
    size_t items = count/sizeof(SomeData);
    bench_results bench_results_buf = bench(iter, [&]() {
        for (size_t i = 0; i < items; ++i)
        {
            buf->Push(tmp_);
        }
        KEEP_ALIVE(buf->Data);
    }, [&](){
        for (size_t i = 0; i < items; ++i)
        {
            buf->Push(tmp_);
        }
        KEEP_ALIVE(cbuf->Data);
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        for (size_t i = 0; i < items; ++i)
        {
            cbuf->Push(tmp_);
        }
        KEEP_ALIVE(cbuf->Data);
    }, [&](){
        for (size_t i = 0; i < items; ++i)
        {
            cbuf->Push(tmp_);
        }
        KEEP_ALIVE(cbuf->Data);
    });

    printf("  Buffer best run:\n");
    clean_results(&bench_results_buf, count);

    printf("  CBuffer best run:\n");
    clean_results(&bench_results_cbuf, count);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

// Reads with a wraparound, accumulating a checksum.
// 
// `iter` how many iterations
// `count` is length of buffers (bytes)
bench_results_bufs bench_wraparound_read_byte(ByteBuffer* buf, CByteBuffer* cbuf, size_t count, size_t iter)
{
    // Both buffers are already prefilled
    size_t items = count/sizeof(SomeData);
    size_t expected_sum = items * tmp_.d;

    printf("\nWraparound read, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = 0; i < items; ++i)
        {
            sum += buf->Pop<SomeData>().d;
        }
        KEEP_ALIVE(sum);
        assert(expected_sum == sum);
    }, [&](){
        for (size_t i = 0; i < items; ++i)
        {
            buf->Push(tmp_);
        }
        KEEP_ALIVE(cbuf->Data);
        buf->Tail = count/2; // set tail in the middle, to force wraparound
    });

    bench_results bench_results_cbuf = bench(iter, [&]() {
        int64_t sum = 0;
        for (size_t i = 0; i < items; ++i)
        {
            sum += cbuf->Pop<SomeData>().d;
        }
        KEEP_ALIVE(sum);
        assert(expected_sum == sum);
    }, [&](){
        for (size_t i = 0; i < items; ++i)
        {
            cbuf->Push(tmp_);
        }
        KEEP_ALIVE(cbuf->Data);
        buf->Tail = count/2; // set tail in the middle, to force wraparound
    });

    printf("  Buffer best run:\n");
    clean_results(&bench_results_buf, count);

    printf("  CBuffer best run:\n");
    clean_results(&bench_results_cbuf, count);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

// `iter` how many iterations
// `count` is length of buffers (bytes)
bench_results_bufs bench_alternate_read_write_byte(ByteBuffer* buf, CByteBuffer* cbuf, size_t count, size_t iter)
{
    // Both buffers are already prefilled
    size_t items = count/sizeof(SomeData);
    size_t alt_every = 16;
    size_t expected_alt_sum = alt_every * tmp_.d;
    size_t expected_tot_sum = items * tmp_.d;

    printf("\nAlternate read/write, buffer size: %ld\n", count);
    bench_results bench_results_buf = bench(iter, [&]() {
        int64_t tot_sum = 0;
        for (size_t i = 0; i < items/alt_every; ++i)
        {
            int64_t alt_sum = 0;
            for (size_t i = 0; i < alt_every; ++i)
            {
                buf->Push(tmp_);
            }
            for (size_t i = 0; i < alt_every; ++i)
            {
                alt_sum += buf->Pop<SomeData>().d;                
            }
            KEEP_ALIVE(alt_sum);
            KEEP_ALIVE(tot_sum);
            assert(expected_alt_sum == alt_sum);
            tot_sum += alt_sum;
        }
        assert(expected_tot_sum == tot_sum);
    }, [&](){});

    bench_results bench_results_cbuf = bench(iter, [&]() {
        int64_t tot_sum = 0;
        for (size_t i = 0; i < items/alt_every; ++i)
        {
            int64_t alt_sum = 0;
            for (size_t i = 0; i < alt_every; ++i)
            {
                cbuf->Push(tmp_);
            }
            for (size_t i = 0; i < alt_every; ++i)
            {
                alt_sum += cbuf->Pop<SomeData>().d;                
            }
            KEEP_ALIVE(alt_sum);
            KEEP_ALIVE(tot_sum);
            assert(expected_alt_sum == alt_sum);
            tot_sum += alt_sum;
        }
        assert(expected_tot_sum == tot_sum);
    }, [&](){});

    printf("  Buffer best run:\n");
    clean_results(&bench_results_buf, count);

    printf("  CBuffer best run:\n");
    clean_results(&bench_results_cbuf, count);

    return (bench_results_bufs){ bench_results_cbuf.metric, bench_results_buf.metric };
}

void byte_buffer_benchmark() {
    int i;
    int tests = 5;
    int loops = 7; //   4k    64k      512k      4m         8m         16m        256m
    size_t bytes[7] = {4096, 16*4096, 128*4096, 1024*4096, 2048*4096, 4096*4096, 16*4096*4096};
    size_t iters[7] = {100000, 10000,  10000,     10000,      1000,      500,       100};

    bench_results_bufs bench_results_metrics[tests*loops];
    for (i = 0; i < loops; ++i)
    {
        ByteBuffer buf(bytes[i]);
        CByteBuffer cbuf(bytes[i]);
        bench_results_metrics[0+tests*i] = bench_sequential_write_byte(&buf, &cbuf, bytes[i], iters[i]);
        buf.Reset(); cbuf.Reset();
        bench_results_metrics[1+tests*i] = bench_sequential_read_byte(&buf, &cbuf, bytes[i], iters[i]);
        buf.Reset(); cbuf.Reset();
        bench_results_metrics[2+tests*i] = bench_wraparound_write_byte(&buf, &cbuf, bytes[i], iters[i]);
        buf.Reset(); cbuf.Reset();
        bench_results_metrics[3+tests*i] = bench_wraparound_read_byte(&buf, &cbuf, bytes[i], iters[i]); // fails
        buf.Reset(); cbuf.Reset();
        bench_results_metrics[4+tests*i] = bench_alternate_read_write_byte(&buf, &cbuf, bytes[i], iters[i]);
    }
    
    printf("bytes,buf_seq_w,cbuf_seq_w,buf_seq_r,cbuf_seq_r,buf_wrap_w,cbuf_wrap_w,buf_wrap_r,cbuf_wrap_r,buf_alt,cbuf_alt\n");
    for (i = 0; i < loops; ++i) {
        printf("%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n",bytes[i],
            bench_results_metrics[0+tests*i].buf_metric, bench_results_metrics[0+tests*i].cbuf_metric,
            bench_results_metrics[1+tests*i].buf_metric, bench_results_metrics[1+tests*i].cbuf_metric,
            bench_results_metrics[2+tests*i].buf_metric, bench_results_metrics[2+tests*i].cbuf_metric,
            bench_results_metrics[3+tests*i].buf_metric, bench_results_metrics[3+tests*i].cbuf_metric,
            bench_results_metrics[4+tests*i].buf_metric, bench_results_metrics[4+tests*i].cbuf_metric
        );
    }
}

int main()
{
    // typed_buffer_benchmark();
    byte_buffer_benchmark();

    return 0;
}
