# Circular Buffers

This project provides circular buffer implementations, with virtual memory aliasing / address space mirroring.

## buffer.hpp

### Buffer<T>
Typed circular buffer.
- `Buffer(size_t count)`: Allocate memory for `count` items.
- `operator[]`: Access item at index. Logic wraps indices to buffer range.
- `GetSize()`: Return size in bytes.

### ByteBuffer
Byte-oriented circular buffer.
- `ByteBuffer(size_t capacity)`: Allocate memory for `capacity` bytes.
- `Push<T>(const T& data)`: Put `data` at head.
- `Pop<T>()`: Get `T` at tail.
- `Reset()`: Set head and tail to zero.

#### Usage
```cpp
Buffer<int> buf(1024);
buf[0] = 42;

ByteBuffer bytes(2048);
bytes.Push(100L);
long val = bytes.Pop<long>();
```

## cbuffer.hpp

### CBuffer<T>
Typed buffer with address mirroring. Logic maps physical pages to adjacent virtual addresses. This allows access beyond the buffer limit without masking.
- `CBuffer(size_t size)`: Allocate buffer. `size` becomes a multiple of page size.
- `operator[]`: Access item at index. Memory mapping handles wraparound.

### CByteBuffer
Byte-oriented buffer with address mirroring.
- `CByteBuffer(size_t size)`: Allocate buffer.
- `Push<T>(const T& data)`: Put `data` at head.
- `Pop<T>()`: Get `T` at tail.

#### Usage
```cpp
CBuffer<int> cbuf(4096);
cbuf[10000] = 5; // Maps to within buffer

CByteBuffer cbytes(4096);
cbytes.Push(1.5f);
float f = cbytes.Pop<float>();
```

## Benchmark
`benchmark.cpp` compares throughput for both implementations. It tests read and write speeds across various scales. Results appear in stdout as GiB/s.

`buff_bench.ods` contains charts and data from these benchmarks.
