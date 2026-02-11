#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <type_traits>
#include <stdexcept>

size_t BitCeil(size_t v)
{
    if (v <= 1)
    {
        return 1;
    }
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

// Circular Buffer of (probably) 4kb, but feels way bigger.
// And it's super efficient, as it leverages CUPs and
// RAM's native ops to do the hard work.
//
// VSize: how big the buffer "feels like", will be >= PSize.
// PSize: how big the buffer actually is. By default (and as a minimum)
//        we use your system's page size: sysconf(_SC_PAGESIZE)
template <typename T>
class CBuffer
{
    static_assert(std::is_trivially_copyable<T>::value,
                  "CBuffer requires a trivially copyable type.");

public:
    size_t VSize; // Percieved buffer size (>= PSize)
    size_t PSize; // Physical buffer size (multiple of 4096)
    T *Data;      // Buffer

    // Virtual buffer: how big the buffer "feels like", how
    CBuffer(size_t vbuffer_size_) : VSize(BitCeil(vbuffer_size_)),
                                    PSize(sysconf(_SC_PAGESIZE))
    {
        Allocate();
    };

    // Custom pbuffer
    CBuffer(size_t vbuffer_size_,
            size_t pbuffer_size_) : VSize(BitCeil(vbuffer_size_)),
                                    PSize(BitCeil(pbuffer_size_))
    {
        if (PSize < sysconf(_SC_PAGESIZE))
        {
            PSize = sysconf(_SC_PAGESIZE);
        }
        Allocate();
    };

    void Allocate()
    {
        if (VSize < PSize)
        {
            VSize = PSize;
        }

        void *Base = mmap(NULL, VSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        Data = (T *)Base;

        int fd = memfd_create("cbuffer", 0);
        ftruncate(fd, PSize);

        for (int i = 0; i < GetPageCount(); ++i)
        {
            void *addr = (char *)Base + (i * PSize);
            mmap(addr, PSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
        }
        close(fd);
    };

    ~CBuffer()
    {
        if (Data)
        {
            if (munmap(Data, VSize) == -1)
            {
                fprintf(stderr, "ERROR: Unable to unallocate c_buffer");
            }
        }
    };

    // How many items fit in your virtual buffer
    size_t GetVItemCount()
    {
        return VSize / sizeof(T);
    };

    // How many items fit in your physical buffer
    size_t GetPItemCount()
    {
        return PSize / sizeof(T);
    };

    // Virtual pages over your physical page
    int GetPageCount()
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

    T &at(size_t index)
    {
        if (index >= GetVItemCount())
        {
            throw std::out_of_range("CBuffer index out of range");
        }
        return Data[index];
    };

    const T &at(size_t index) const
    {
        if (index >= GetVItemCount())
        {
            throw std::out_of_range("CBuffer index out of range");
        }
        return Data[index];
    };
};

int main()
{
    CBuffer<uint32_t> myBuffer(32 * 1024 * 1024, 2 * 4096);
    return 0;
}
