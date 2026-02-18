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
#include "cbuffer.hpp"

// default page size
CBuffer<uint8_t> buf();

// Fast unchecked access
buf[100] = 42;

// Hardware wrap-around: indexing beyond the physical page wraps to the same backing page
buf[100 + buf.GetPItemCount()] = 7; // writes the same physical memory as buf[100]

// Bounds-checked access
try {
    auto &v = buf.at(buf.GetVItemCount() - 1);
} catch (const std::out_of_range &e) {
    // handle
}
```

- Constructors (overloads):
	- `CBuffer()` everything default: `PSize = sysconf(_SC_PAGESIZE)`, `VSize = 16 * PSize`.
	- `CBuffer(size_t pbuffer_size_)` set `PSize` (rounded up to next page), `VSize = 16 * PSize`.
	- `CBuffer(uint8_t vbuffer_mult_)` set `PSize = page size`, `VSize = vbuffer_mult_ * PSize`.
	- `CBuffer(size_t pbuffer_size_, uint8_t vbuffer_mult_)` custom everything: `PSize` and virtual multiplier.

### Safety Features

The library uses `std::is_trivially_copyable` to ensure only valid data types are used. It provides two access methods:

* `operator[]`: Maximum speed, no bounds checking.
* `at()`: Throws `std::out_of_range` if the index exceeds the total **virtual** size.



