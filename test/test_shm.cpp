#include "shared_memory.hpp"
#include <vector>
#include <map>
#include <gtest/gtest.h>
#include "test_header.h"
#include "shared_allocator.h"

namespace levin {

class ShmTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
    const size_t fixed_len = SharedAllocator::Allocsize(sizeof(SharedMeta));
};

// 获取系统中
TEST_F(ShmTest, test_get_all_shmid) {
    std::vector<SharedMidInfo> map_shmid;
    bool res = SharedMemory::get_all_shmid(map_shmid);
    EXPECT_TRUE(res);
}

// 创建共享内存超过限制最大值
TEST_F(ShmTest, test_exceed_the_maximun) {
    std::string file = "/tmp";
    const size_t memsize = 60000000000;
    auto sm_ptr = new SharedMemory(file, 1, memsize);
    uint32_t res = sm_ptr->init(fixed_len);
    EXPECT_EQ(res, SC_RET_SHM_SIZE_ERR);
    delete sm_ptr;
}
// 不存在文件创建共享内存
TEST_F(ShmTest, test_no_exist_file) {
    std::string file = "./no_exist.file";
    const size_t memsize = 6000;
    SharedMemory shm(file, 1, memsize);
    uint32_t res = shm.init(fixed_len);
    EXPECT_EQ(res, SC_RET_FILE_NOEXIST);
}

// 对象接口删除共享内存
TEST_F(ShmTest, test_remove) {
    std::string file = "/tmp";
    const size_t memsize = 6000;
    SharedMemory shm(file, 1, memsize);
    uint32_t res = shm.init(fixed_len);
    EXPECT_EQ(res, SC_RET_OK);
    EXPECT_TRUE(shm.remove());
}
// 删除指定id共享内存
TEST_F(ShmTest, test_remove_shmid) {
    std::string file = "/tmp";
    const size_t memsize = 6000;
    SharedMemory shm(file, 2, memsize);
    uint32_t res = shm.init(fixed_len);
    EXPECT_EQ(res, SC_RET_OK);
    int shmid = shm.get_shmid();
    EXPECT_TRUE(SharedMemory::remove_shared_memory(shmid));
}
// 删除指定id共享内存
TEST_F(ShmTest, test_existed_shm) {
    std::string file = "/tmp";
    const size_t memsize = 6000;
    SharedMemory shm(file, 3, memsize);
    uint32_t res = shm.init(fixed_len);
    EXPECT_EQ(res, SC_RET_OK);
    EXPECT_FALSE(shm.is_exist());
    res = shm.init(fixed_len);
    EXPECT_EQ(res, SC_RET_SHM_KEY_CONFLICT);
    EXPECT_TRUE(shm.is_exist());
    int shmid = shm.get_shmid();
    bool flag = SharedMemory::remove_shared_memory(shmid);
    EXPECT_TRUE(flag);
}

}  // namespace levin

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
