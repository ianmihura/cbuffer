#include <gtest/gtest.h>
#include "cbuffer.hpp"

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
