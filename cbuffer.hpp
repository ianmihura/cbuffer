#ifndef C_BUFFER_HPP
#define C_BUFFER_HPP

#include <stdio.h>
#include <cstdio>
#include <stdint.h>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <type_traits>
#include <stdexcept>
#include <cassert>
#include <functional>
#include <x86intrin.h>
#include <sys/time.h>


inline size_t ToNextPageSize(size_t v)
{
    const size_t PAGE_SIZE = sysconf(_SC_PAGESIZE);
    if (v < PAGE_SIZE)
    {
        return PAGE_SIZE;
    }
    else
    {
        return ((v + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    }
}

// Circular Buffer of (probably) 4kb, but feels way bigger.
// It leverages CUP and RAM's native ops to do the hard work.
//
// VSize: Virtual buffer size. how big the buffer "feels like", default: 16x the size of the underlying physical buffer. Must be a multiple of 4096.
// PSize: Physical buffer size. how big the buffer actually is. By default (and as a minimum)
//        we use your system's page size: sysconf(_SC_PAGESIZE)
template <typename T>
class CBuffer
{
    static_assert(std::is_trivially_copyable<T>::value,
                  "CBuffer requires a trivially copyable type.");

public:
    size_t PSize; // Physical buffer size (multiple of your page size, probably 4096)
    size_t VSize; // Virtual buffer size, how much the buffer actually feels like (>= PSize)
    T *Data;      // Buffer

    // Physical size is one page, usually 4096 (default)
    // Virtual size is 16x size of the physical buffer (default)
    CBuffer() : PSize(sysconf(_SC_PAGESIZE)),
                VSize(16*PSize)
    {
        Allocate();
    };

    // Custom Physical size (must be multiple of page size)
    // Virtual size is 16x size of the physical buffer (default)
    CBuffer(size_t pbuffer_size_) : PSize(ToNextPageSize(pbuffer_size_)),
                                    VSize(16*PSize)
    {
        Allocate();
    };

    // Custom Physical size (must be multiple of page size)
    // Custom Virtual size multiplier: is x times sizes of the physical buffer
    CBuffer(size_t pbuffer_size_,
            uint8_t vbuffer_mult_) : PSize(ToNextPageSize(pbuffer_size_)),
                                     VSize(vbuffer_mult_*PSize)
    {
        Allocate();
    };

    ~CBuffer()
    {
        if (Data != nullptr)
        {
            if (munmap(Data, VSize) == -1)
            {
                const char *error_msg = strerror(errno);
                fprintf(stderr, "CBuffer Cleanup Error: %s\n", error_msg);
            }
            Data = nullptr;
        }
    };

    CBuffer(const CBuffer &) = delete;
    CBuffer &operator=(const CBuffer &) = delete;

    // How many items fit in your virtual buffer
    size_t GetVItemCount() const
    {
        return VSize / sizeof(T);
    };

    // How many items fit in your physical buffer
    size_t GetPItemCount() const
    {
        return PSize / sizeof(T);
    };

    // Virtual pages over your physical page
    size_t GetPageCount() const
    {
        return VSize / PSize;
    };

    T &operator[](size_t index)
    {
        return Data[index];
    };

    const T &operator[](size_t index) const
    {
        return Data[index];
    };

private:
    void Allocate()
    {
        if (VSize < PSize)
        {
            VSize = PSize;
        }

        void *Base = mmap(NULL, VSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (Base == MAP_FAILED)
        {
            throw std::runtime_error("Virtual reservation failed");
        }
        Data = static_cast<T *>(Base);

        int fd = memfd_create("cbuffer", 0);
        if (fd == -1)
        {
            throw std::runtime_error("memfd_create failed");
        }
        ftruncate(fd, PSize);

        for (size_t i = 0; i < GetPageCount(); ++i)
        {
            void *addr = (char *)Base + (i * PSize);
            if (mmap(addr, PSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) == MAP_FAILED)
            {
                close(fd);
                throw std::runtime_error("Physical mapping failed");
            }
        }
        close(fd);
    };
};

// Generic Buffer of (probably) 4kb, but feels way bigger.
// It leverages CUP and RAM's native ops to do the hard work.
//
// VSize: Virtual buffer size. how big the buffer "feels like", default: 4GB. Previously: 16x the size of the underlying physical buffer. Must be a multiple of 4096.
// PSize: Physical buffer size. how big the buffer actually is. By default (and as a minimum)
//        we use your system's page size: sysconf(_SC_PAGESIZE)
class CByteBuffer
{
public:
    size_t PSize;    // Physical buffer size (multiple of your page size, probably 4096)
    size_t VSize;  // Virtual buffer size, how much the buffer actually feels like (>= PSize)
    std::byte *Data; // Buffer
    uint64_t Head;   // Buffer Head
    uint64_t Tail;   // Buffer Tail

    // Physical size is one page, usually 4096 (default)
    // Virtual size is 4GB (default)
    CByteBuffer() : PSize(sysconf(_SC_PAGESIZE)),
                    VSize(4294967296)
                    // VSize(16*PSize)
    {
        Allocate();
    };

    // Custom Physical size (must be multiple of page size)
    // Virtual size is 4GB (default)
    CByteBuffer(size_t pbuffer_size_) : PSize(ToNextPageSize(pbuffer_size_)),
                                        VSize(4294967296)
                                        // VSize(16*PSize)
    {
        Allocate();
    };

    // Custom Physical size (must be multiple of page size)
    // Custom Virtual size multiplier: is x times sizes of the physical buffer
    CByteBuffer(size_t pbuffer_size_,
            uint8_t vbuffer_mult_) : PSize(ToNextPageSize(pbuffer_size_)),
                                     VSize(vbuffer_mult_*PSize)
    {
        Allocate();
    };

    ~CByteBuffer()
    {
        if (Data != nullptr)
        {
            if (munmap(Data, VSize) == -1)
            {
                const char *error_msg = strerror(errno);
                fprintf(stderr, "CByteBuffer Cleanup Error: %s\n", error_msg);
            }
            Data = nullptr;
        }
    };

    void Reset()
    {
        Head=0;
        Tail=0;
    };

    CByteBuffer(const CByteBuffer &) = delete;
    CByteBuffer &operator=(const CByteBuffer &) = delete;

    // Virtual pages over your physical page
    size_t GetPageCount() const
    {
        return VSize / PSize;
    };

    std::byte &operator[](size_t index)
    {
        return Data[index];
    };

    const std::byte &operator[](size_t index) const
    {
        return Data[index];
    };


template <typename T>
void Push(const T& data) {
    static_assert(std::is_trivially_copyable_v<T>);
    
    *reinterpret_cast<T*>(&Data[Head]) = data;
    Head += sizeof(T);

    if (__builtin_expect(Head >= VSize, 0))
    {
        Head -= VSize;
    }
};

template <typename T>
T Pop() {
    static_assert(std::is_trivially_copyable_v<T>);

    T data;
    std::memcpy(&data, &Data[Tail], sizeof(T));
    Tail += sizeof(T);

    if (__builtin_expect(Tail >= VSize, 0))
    {
        Tail -= VSize;
    }

    return data;
};

private:
    void Allocate()
    {
        Head = 0;
        Tail = 0;
        if (VSize < PSize)
        {
            VSize = PSize;
        }
        if (PSize == 4096) VSize = 4294803456; // hotfix: my cpu is not allowing bigger VSize

        void *Base = mmap(NULL, VSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (Base == MAP_FAILED)
        {
            throw std::runtime_error("Virtual reservation failed");
        }
        Data = static_cast<std::byte*>(Base);

        int fd = memfd_create("CByteBuffer", 0);
        if (fd == -1)
        {
            throw std::runtime_error("memfd_create failed");
        }
        ftruncate(fd, PSize);

        for (size_t i = 0; i < GetPageCount(); ++i)
        {
            void *addr = (char *)Base + (i * PSize);
            if (mmap(addr, PSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) == MAP_FAILED)
            {
                munmap(Data, VSize); // try to unmap, otherwise will not exit probram
                close(fd);
                throw std::runtime_error("Physical mapping failed");
            }
        }
        close(fd);
    };
};

#endif
