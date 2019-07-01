#include "shared_allocator.h"
#include <vector>
#include <gtest/gtest.h>
#include "test_header.h"

namespace levin {

class SharedAllocatorTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(SharedAllocatorTest, test_Allocsize) {
    EXPECT_EQ(levin::SharedAllocator::Allocsize(0), 0);
    EXPECT_EQ(levin::SharedAllocator::Allocsize(1), 8);
    EXPECT_EQ(levin::SharedAllocator::Allocsize(4), 8);
    EXPECT_EQ(levin::SharedAllocator::Allocsize(8), 8);
    EXPECT_EQ(levin::SharedAllocator::Allocsize(13), 16);
}

TEST_F(SharedAllocatorTest, test_Address) {
    const size_t mem_capacity = 256;
    void *addr = malloc(mem_capacity);
    SharedAllocator alloc(addr, mem_capacity);
    EXPECT_EQ(alloc._base_addr, addr);
    EXPECT_EQ(alloc._capacity, mem_capacity);
    EXPECT_EQ(alloc._used_size, 0);
    // succ
    {
        uint64_t *ret = alloc.Address<uint64_t>();
        EXPECT_EQ((void*)ret, addr);
        EXPECT_EQ(alloc._capacity, mem_capacity);
        EXPECT_EQ(alloc._used_size, sizeof(uint64_t));

        std::pair<int, uint64_t> *ret1 = alloc.Address<std::pair<int, uint64_t> >();
        EXPECT_EQ((size_t)ret1, (size_t)addr + sizeof(uint64_t));
        EXPECT_EQ(alloc._capacity, mem_capacity);
        EXPECT_EQ(alloc._used_size, sizeof(uint64_t) + sizeof(std::pair<int, uint64_t>));
    }
    // fail
    {
        EXPECT_THROW(alloc.Address<char[256]>(), std::runtime_error);
    }
    free(addr);
}

TEST_F(SharedAllocatorTest, test_Construct) {
    const size_t mem_capacity = 256;
    void *addr = malloc(mem_capacity);
    SharedAllocator alloc(addr, mem_capacity);
    // succ
    {
        Cat *hellokitty = alloc.Construct<Cat>("kitty");
        EXPECT_EQ((void*)hellokitty, addr);
        EXPECT_EQ(alloc._used_size, SharedAllocator::Allocsize(sizeof(Cat)));
        EXPECT_STREQ(hellokitty->name, "kitty");
        hellokitty->~Cat();

        const size_t CAT_NUM = 10;
        Cat *cats = alloc.ConstructN<Cat>(CAT_NUM);
        EXPECT_EQ((size_t)cats, (size_t)addr + SharedAllocator::Allocsize(sizeof(Cat)));
        EXPECT_STREQ(cats[0].name, "");
        EXPECT_EQ(
                alloc._used_size,
                SharedAllocator::Allocsize(sizeof(Cat)) + SharedAllocator::Allocsize(sizeof(Cat) * CAT_NUM));
    }
    // fail
    {
        EXPECT_THROW(alloc.ConstructN<Cat>(10), std::runtime_error);
    }
    free(addr);
}

TEST_F(SharedAllocatorTest, test_OutOfRange) {
    const size_t mem_capacity = 256;
    void *addr = malloc(mem_capacity);
    SharedAllocator alloc(addr, mem_capacity);
    EXPECT_FALSE(alloc.OutOfRange(addr, mem_capacity));
    EXPECT_TRUE(alloc.OutOfRange((void*)((size_t)addr + 1), mem_capacity - 1));
    EXPECT_TRUE(alloc.OutOfRange(addr, mem_capacity + 1));
    EXPECT_TRUE(alloc.OutOfRange((void*)((size_t)addr + 1), mem_capacity));
    EXPECT_TRUE(alloc.OutOfRange((void*)((size_t)addr - 1), mem_capacity));

    free(addr);
}

}  // namespace levin

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
