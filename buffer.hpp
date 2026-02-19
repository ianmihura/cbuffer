#ifndef BUFFER_HPP
#define BUFFER_HPP

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

    ByteBuffer(const ByteBuffer &) = delete;
    ByteBuffer &operator=(const ByteBuffer &) = delete;

    std::byte &operator[](size_t index)
    {
        return Data[index % Capacity];
    };

    const std::byte &operator[](size_t index) const
    {
        return Data[index % Capacity];
    };

    template <typename T>
    void Push(const T& data) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be safe to copy via memory");
    
        const std::byte* src = reinterpret_cast<const std::byte*>(&data);
        size_t len = sizeof(T);

        if (Head + len <= Capacity) {
            // no wrapping
            std::memcpy(&Data[Head], src, len);
            Head += len;
        } else {
            // wrapping
            size_t firstPart = Capacity - Head;
            size_t secondPart = len - firstPart;
            
            std::memcpy(&Data[Head], src, firstPart);
            std::memcpy(&Data[0], src + firstPart, secondPart);
            Head = (Head + len) & Capacity-1;
        }
    };

    template <typename T>
    T Pop() {
        T data;

        if (Tail + sizeof(T) <= Capacity) {
            std::memcpy(&data, &Data[Tail], sizeof(T));
            Tail += sizeof(T);
        } else {
            // wrapping
            size_t firstPart = Capacity - Tail;
            size_t secondPart = sizeof(T) - firstPart;
            std::memcpy(&data, &Data[Tail], firstPart);
            std::memcpy(&data + firstPart, &Data[0], secondPart);
            Tail = (Tail + sizeof(T)) & Capacity-1;
        }

        return data;
    };
};

#endif
