#include <gtest/gtest.h>
#include "cbuffer.hpp"

// Test that the memory actually mirrors
TEST(CBufferTest, VirtualAliasing) {
    const size_t page_size = sysconf(_SC_PAGESIZE);
    // Create a buffer that mirrors a 4KB physical page twice
    CBuffer<int> buf(page_size * 2, page_size);

    int items_per_page = page_size / sizeof(int);

    // Write to the first "page"
    buf[0] = 1234;

    // Check the second "page" (the alias)
    EXPECT_EQ(buf[items_per_page], 1234);

    // Modify the alias and check the original
    buf[items_per_page] = 5678;
    EXPECT_EQ(buf[0], 5678);
}

// Test bounds checking
TEST(CBufferTest, BoundsChecking) {
    CBuffer<int> buf(4096 * 2);
    size_t max_items = buf.VSize / sizeof(int);

    EXPECT_NO_THROW(buf.at(max_items - 1));
    EXPECT_THROW(buf.at(max_items), std::out_of_range);
}

// Test that it handles non-power-of-two inputs by rounding
TEST(CBufferTest, RoundingLogic) {
    CBuffer<int> buf(5000); // Should round up to 8192 if page is 4k
    EXPECT_GE(buf.VSize, 5000);
    EXPECT_EQ(buf.VSize % sysconf(_SC_PAGESIZE), 0);
}
