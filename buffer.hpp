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

    // Size of the buffer in bytes
    size_t GetSize()
    {
        return Count * sizeof(T);
    }

    T &operator[](size_t index)
    {
        return Data[index % Count];
    };

    const T &operator[](size_t index) const
    {
        return Data[index % Count];
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
        } else {
            // wrapping
            size_t firstPart = Capacity - Head;
            size_t secondPart = len - firstPart;
            
            std::memcpy(&Data[Head], src, firstPart);
            std::memcpy(&Data[0], src + firstPart, secondPart);
        }

        Head = (Head + len) % Capacity;
    };

    template <typename T>
    T Pop() {
        T data;
        size_t len = sizeof(T);
        std::byte* dest = reinterpret_cast<std::byte*>(&data);

        if (Tail + len <= Capacity) {
            // no wrapping
            std::memcpy(dest, &Data[Tail], len);
        } else {
            // wrapping
            size_t firstPart = Capacity - Tail;
            size_t secondPart = len - firstPart;
            std::memcpy(dest, &Data[Tail], firstPart);
            std::memcpy(dest + firstPart, &Data[0], secondPart);
        }

        Tail = (Tail + len) % Capacity;
        return data;
    };
};

#endif
