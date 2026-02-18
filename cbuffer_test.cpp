#include <gtest/gtest.h>
#include "cbuffer.hpp"
#include "buffer.hpp"

// Test that the memory actually mirrors
TEST(CBufferTest, VirtualAliasing) {
    const size_t page_size = sysconf(_SC_PAGESIZE);
    // Create a buffer that mirrors a 4KB physical page twice
    CBuffer<int> buf(page_size);

    int items_per_page = page_size / sizeof(int);

    // Write to the first "page"
    buf[0] = 1234;

    // Check the second "page" (the alias)
    EXPECT_EQ(buf[items_per_page], 1234);

    // Modify the alias and check the original
    buf[items_per_page] = 5678;
    EXPECT_EQ(buf[0], 5678);
}

// Test ToNextPageSize
TEST(CBufferTest, ToNextPageSize) {
    auto next = ToNextPageSize(5000); // Should round up to 8192 if page is 4k
    EXPECT_EQ(next, 4096*2);
    next = ToNextPageSize(50000);
    EXPECT_EQ(next, 4096*13);
}

struct Sarasa
{
    uint32_t a;
    uint32_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    bool f;
    bool g;
};

TEST(CByteBufferTest, GeneralTest) {
    Sarasa t_ = {918243,123443,12,61,0,true,true};
    Sarasa a_ = {15114,6124,62,9,245,false,true};
    CByteBuffer bbuf;
    bbuf.Push<Sarasa>(a_);
    bbuf.Push<Sarasa>(t_);
    bbuf.Pop<Sarasa>(); // FIFO (a_)
    auto t_bis = bbuf.Pop<Sarasa>();

    EXPECT_EQ(t_bis.a, t_.a);
    EXPECT_EQ(t_bis.b, t_.b);
    EXPECT_EQ(t_bis.c, t_.c);
    EXPECT_EQ(t_bis.d, t_.d);
    EXPECT_EQ(t_bis.e, t_.e);
    EXPECT_EQ(t_bis.f, t_.f);
    EXPECT_EQ(t_bis.g, t_.g);
}

TEST(CByteBufferTest, Wraparound) {
    Sarasa t_ = {918243,123443,12,61,0,true,true};
    Sarasa a_ = {15114,6124,62,9,245,false,true};
    CByteBuffer bbuf;
    bbuf.Push<Sarasa>(a_);
    for (int i = 0; i < 1024; ++i) {
        bbuf.Push<Sarasa>(t_);
    }
    auto t_bis = bbuf.Pop<Sarasa>(); // a_ will be overwritten with t_

    EXPECT_EQ(t_bis.a, t_.a);
    EXPECT_EQ(t_bis.b, t_.b);
    EXPECT_EQ(t_bis.c, t_.c);
    EXPECT_EQ(t_bis.d, t_.d);
    EXPECT_EQ(t_bis.e, t_.e);
    EXPECT_EQ(t_bis.f, t_.f);
    EXPECT_EQ(t_bis.g, t_.g);
}

TEST(ByteBufferTest, GeneralTest) {

    Sarasa t_ = {918243,123443,12,61,0,true,true};
    Sarasa a_ = {15114,6124,62,9,245,false,true};
    ByteBuffer bbuf(4096);
    bbuf.Push<Sarasa>(a_);
    bbuf.Push<Sarasa>(t_);
    bbuf.Pop<Sarasa>(); // FIFO (a_)
    auto t_bis = bbuf.Pop<Sarasa>();

    EXPECT_EQ(t_bis.a, t_.a);
    EXPECT_EQ(t_bis.b, t_.b);
    EXPECT_EQ(t_bis.c, t_.c);
    EXPECT_EQ(t_bis.d, t_.d);
    EXPECT_EQ(t_bis.e, t_.e);
    EXPECT_EQ(t_bis.f, t_.f);
    EXPECT_EQ(t_bis.g, t_.g);
}

TEST(ByteBufferTest, Wraparound) {

    Sarasa t_ = {918243,123443,12,61,0,true,true};
    Sarasa a_ = {15114,6124,62,9,245,false,true};
    ByteBuffer bbuf(4096);
    bbuf.Push<Sarasa>(a_);
    for (int i = 0; i < 1024; ++i) {
        bbuf.Push<Sarasa>(t_);
    }
    auto t_bis = bbuf.Pop<Sarasa>(); // a_ will be overwritten with t_

    EXPECT_EQ(t_bis.a, t_.a);
    EXPECT_EQ(t_bis.b, t_.b);
    EXPECT_EQ(t_bis.c, t_.c);
    EXPECT_EQ(t_bis.d, t_.d);
    EXPECT_EQ(t_bis.e, t_.e);
    EXPECT_EQ(t_bis.f, t_.f);
    EXPECT_EQ(t_bis.g, t_.g);
}
