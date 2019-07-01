#include "shared_base.hpp"
#include "svec.hpp"
#include <vector>
#include <gtest/gtest.h>
#include "test_header.h"

namespace levin {

class SharedBaseTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

static const std::string name = "./intvec.dat";
void InitEnv() {
//    CleanEnvShm(std::vector<std::string>({levin::name}));
    levin::SharedVector<int>::Dump(name, std::vector<int>({0, 1, 2, 3, 4, 5}));
}

class MockSharedMemory : public SharedMemory {
public:
    MockSharedMemory(const std::string &path) : SharedMemory(path) {
    }
    virtual ~MockSharedMemory() {
    }
};

TEST_F(SharedBaseTest, test__init) {
    // SC_RET_FILE_NOEXIST
    {
        std::string name = "./noexist.dat";
        levin::SharedVector<int> vec(name);
        EXPECT_EQ(vec._init(vec._object), SC_RET_FILE_NOEXIST);
    }
    // SC_RET_SHM_SIZE_ERR
    {
        std::string name = "./exceed.dat";
        std::ofstream fout(name, std::ios::out | std::ios::binary);
        size_t mem_size = MAX_MEM_SIZE;
        fout.write((const char*)&mem_size, sizeof(mem_size));
        fout.close();

        levin::SharedVector<int> vec(name);
        EXPECT_EQ(vec._init(vec._object), SC_RET_SHM_SIZE_ERR);
    }
    // SC_RET_READ_FAIL
    {
        std::string name = "./readfail.dat";
        std::ofstream fout(name, std::ios::out | std::ios::binary);
        char mem_size = 0;
        fout.write((const char*)&mem_size, sizeof(mem_size));
        fout.close();

        levin::SharedVector<int> vec(name);
        EXPECT_EQ(vec._init(vec._object), SC_RET_READ_FAIL);
    }
#if 0
    // SC_RET_OOM, modify SharedMemory init() to virtual for MOCK success
    {
        std::string name = "./oom.dat";
        levin::SharedVector<int> vec(name);
        auto mockshm_ptr = new MockSharedMemory(name);
        vec._info->_shm.reset(mockshm_ptr);
        EXPECT_CALL(*mockshm_ptr, init())
                .WillOnce(::testing::Return(SC_RET_OOM));
        EXPECT_EQ(vec._init(vec._object), SC_RET_OOM);
    }
#endif
}

TEST_F(SharedBaseTest, test__init_exist) {
    // create an exist one
    {
        levin::SharedVector<int> vec(name);
        if (vec.Init() != SC_RET_OK || (!vec.IsExist() && vec.Load() != SC_RET_OK)) {
            return;
        }
        LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());
        // no vec.Destroy();
    }
    // _init when exist, check fail TODO
    // _init when exist, check success
    {
        levin::SharedVector<int> vec(name);
        EXPECT_EQ(vec._init(vec._object), SC_RET_OK);
        EXPECT_TRUE(vec.IsExist());
        vec.Destroy();
    }
    // _init when noexist, init success
    {
        levin::SharedVector<int> vec(name);
        EXPECT_EQ(vec._init(vec._object), SC_RET_OK);
        EXPECT_FALSE(vec.IsExist());
        vec.Destroy();
    }
}

TEST_F(SharedBaseTest, test__load) {
    // create an exist one
    {
        levin::SharedVector<int> vec(name);
        if (vec.Init() != SC_RET_OK || (!vec.IsExist() && vec.Load() != SC_RET_OK)) {
            return;
        }
        LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());
        // no vec.Destroy();
    }
    // already exist, _load ret succ directly
    {
        levin::SharedVector<int> vec(name);
        EXPECT_EQ(vec._init(vec._object), SC_RET_OK);
        EXPECT_EQ(vec._load(vec._object), SC_RET_OK);
        vec.Destroy();
    }
    // no exist, _load ret succ and update meta checkinfo
    {
        levin::SharedVector<int, levin::Md5Checker> vec(name);
        EXPECT_EQ(vec._init(vec._object), SC_RET_OK);
        EXPECT_EQ(vec._load(vec._object), SC_RET_OK);
        EXPECT_STRNE(vec._info->_meta->checksum, "");
        vec.Destroy();
    }
}

TEST_F(SharedBaseTest, test__check) {
    // create an exist one
    {
        levin::SharedVector<int, levin::IntegrityChecker> vec(name);
        if (vec.Init() != SC_RET_OK || (!vec.IsExist() && vec.Load() != SC_RET_OK)) {
            return;
        }
        // no vec.Destroy();
    }
    // type hash not match, check fail, re construct
    {
        levin::SharedVector<uint64_t, levin::IntegrityChecker> vec(name);
        EXPECT_EQ(vec._init(vec._object), SC_RET_OK);
        vec.Destroy();
    }
    // out of region, check fail TODO

    // create an exist one
    {
        levin::SharedVector<int, levin::IntegrityChecker> vec(name);
        if (vec.Init() != SC_RET_OK || (!vec.IsExist() && vec.Load() != SC_RET_OK)) {
            return;
        }
        LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());
        // no vec.Destroy();
    }
    // md5sum check fail, re construct
    {
        levin::SharedVector<int, levin::Md5Checker> vec(name);
        EXPECT_EQ(vec._init(vec._object), SC_RET_OK);
        vec.Destroy();
    }
}

TEST_F(SharedBaseTest, test__file2bin) {
    // empty file
    {
        std::string name = "./empty.dat";
        {
            std::ofstream fout(name, std::ios::out | std::ios::binary);
            fout.close();
        }
        levin::SharedVector<int> vec(name);
        EXPECT_FALSE(vec._file2bin(name, vec._object));
    }
}

}  // namespace levin

int main(int argc, char** argv) {
    levin::InitEnv();

    testing::InitGoogleTest(&argc, argv);
    return  RUN_ALL_TESTS();
}
