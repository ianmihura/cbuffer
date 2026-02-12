# CBuffer: A Virtual Memory Circular Buffer

`CBuffer` is a header-only C++ library that implements a circular array using **virtual memory aliasing**. By mapping multiple virtual address ranges to the same physical memory, the buffer provides hardware-level wrap-around logic.

## Features

* **Automatic Wrap-Around**: No modulo (`%`) or conditional checks required during access.
* **Zero-Copy Logic**: Physical memory is mirrored across virtual pages.
* **Type Safe**: Generalized via templates and restricted to trivially copyable types.
* **Linux Native**: Leverages `mmap` and `memfd_create` for high performance.

## How it Works

The library reserves a contiguous block of virtual address space. It then maps a smaller physical buffer (backed by RAM) into that space multiple times.

When an index exceeds the size of the physical buffer, the CPU’s Memory Management Unit (MMU) transparently redirects the pointer back to the start of the physical memory.

## Usage

```cpp
// Create a buffer that feels like 1MB but only uses 4KB of physical RAM
CBuffer<uint8_t> buffer(1024 * 1024);

// Normal access
buffer[82] = 12;

// Hardware-level wrap-around
// If PSize is 4096 (1024 floats), this modifies the same memory as buffer[0]
buffer[82 + 1024] = 51;

assert(buffer[0] == 51);

```

### Allocate bigger buffers

```cpp
// Create a buffer that feels like 10MB but only uses 1MB of physical RAM
CBuffer<uint8_t> buffer(10 * 1024 * 1024, 1024 * 1024);
```

### Safety Features

The library uses `std::is_trivially_copyable` to ensure only valid data types are used. It provides two access methods:

* `operator[]`: Maximum speed, no bounds checking.
* `at()`: Throws `std::out_of_range` if the index exceeds the total **virtual** size.

## Requirements

* **OS**: Linux (Kernel 3.17+ for `memfd_create`)
* **Language**: C++20

## Installation

Copy `CBuffer.hpp` into your project. Ensure you link with `-pthread` if using in a multi-threaded environment.
