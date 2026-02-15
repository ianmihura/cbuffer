#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <type_traits>
#include <stdexcept>

// Classic Circular Buffer
//
// PSize: how big the buffer actually is.
template <typename T>
class Buffer
{
    static_assert(std::is_trivially_copyable<T>::value,
                  "Buffer requires a trivially copyable type.");

public:
    int Count; // Max elements in the Buffer
    T *Data;   // Buffer

    Buffer(size_t count_) : Count(count_),
                            Data(static_cast<T *>(std::malloc(Count * sizeof(T)))) {}

    ~Buffer()
    {
        std::free(Data);
    };

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    // TODO implement this version: not count, but size
    // How many items fit in your buffer
    // size_t GetItemCount()
    // {
    //     return Size / sizeof(T);
    // };

    size_t GetSize()
    {
        return Count * sizeof(T);
    }

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
        if (index >= Count)
        {
            throw std::out_of_range("Buffer index out of range");
        }
        return Data[index];
    };

    const T &at(size_t index) const
    {
        if (index >= Count)
        {
            throw std::out_of_range("Buffer index out of range");
        }
        return Data[index];
    };
};

#endif
