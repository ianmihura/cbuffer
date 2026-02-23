#ifndef BUFFER_HPP
#define BUFFER_HPP
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


#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <type_traits>
#include <stdexcept>

// Classic Circular Buffer
template <typename T>
class Buffer
{
    static_assert(std::is_trivially_copyable<T>::value,
                  "Buffer requires a trivially copyable type.");

public:
    size_t Count; // Max elements in the Buffer
    T *Data;      // Buffer

    Buffer(size_t count_) : Count(count_),
                            Data(static_cast<T *>(std::malloc(Count * sizeof(T)))) {}

    ~Buffer()
    {
        std::free(Data);
    };

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    // Size of the buffer in bytes
    size_t GetSize()
    {
        return Count * sizeof(T);
    }

    T &operator[](size_t index)
    {
        if (index < Count)
        {
            return Data[index];
        }
        else
        {
            return Data[index & Count-1];
        }
    };

    const T &operator[](size_t index) const
    {
        if (index < Count)
        {
            return Data[index];
        }
        else
        {
            return Data[index & Count-1];
        }
    };
};

class ByteBuffer
{
public:
    int Capacity;    // Max capacity of Buffer
    std::byte *Data; // Buffer
    size_t Head;     // Buffer Head
    size_t Tail;     // Buffer Tail

    ByteBuffer(size_t Capacity_) : Capacity(Capacity_),
                                   Data(static_cast<std::byte *>(std::malloc(Capacity))),
                                   Head(0),
                                   Tail(0) {}

    ~ByteBuffer()
    {
        std::free(Data);
    };

    void Reset()
    {
        // std::free(Data);
        // Data = static_cast<std::byte *>(std::malloc(Capacity));
        Head=0;
        Tail=0;
    };

    ByteBuffer(const ByteBuffer &) = delete;
    ByteBuffer &operator=(const ByteBuffer &) = delete;

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
        static_assert(std::is_trivially_copyable_v<T>, "Type must be safe to copy via memory");

        if (__builtin_expect(Head + sizeof(T) <= Capacity, 1)) {
            // hot path
            *reinterpret_cast<T*>(&Data[Head]) = data;
            // std::memcpy(&Data[Head], &data, sizeof(T));
            Head += sizeof(T);
            if (Head == Capacity) Head = 0;
        } else {
            // cold path
            const std::byte* src = reinterpret_cast<const std::byte*>(&data);
            size_t firstPart = Capacity - Head;
            size_t secondPart = sizeof(T) - firstPart;

            std::memcpy(&Data[Head], src, firstPart);
            std::memcpy(&Data[0], src + firstPart, secondPart);
            Head = secondPart;
        }
    };

    template <typename T>
    T Pop() {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be safe to copy via memory");

        if (__builtin_expect(Tail + sizeof(T) <= Capacity, 1)) {
            // hot path
            T data = *reinterpret_cast<const T*>(&Data[Tail]);
            // T data;
            // std::memcpy(&data, &Data[Tail], sizeof(T));
            Tail += sizeof(T);
            if (Tail == Capacity) Tail = 0;
            return data;
        } else {
            // cold path
            T data;
            size_t firstPart = Capacity - Tail;
            size_t secondPart = sizeof(T) - firstPart;
            std::memcpy(&data, &Data[Tail], firstPart);
            std::memcpy(reinterpret_cast<std::byte*>(&data) + firstPart, &Data[0], secondPart);
            Tail = secondPart;
            return data;
        }
    };
};

#endif
