#include "shared_manager.h"
#include "shared_utils.h"
#include "smap.hpp"
#include "svec.hpp"
#include "sset.hpp"
#include "shared_memory.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <time.h>
#include <iostream>

namespace levin {

#define TEST_GROUP_ID   "test_group"
#define TEST_APP_ID     1
#define TEST_VEC_PATH   "./test_vec"
#define TEST_MAP_PATH   "./test_map"
#define TEST_SET_PATH   "./test_set"


class SharedManagerTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        std::vector<int> test_vec = {1, 2, 3, 4, 5};
        SharedVector<int>::Dump(TEST_VEC_PATH, test_vec);
        std::map<int, int> test_map = {{1,10}, {2,20}, {3,30}};
        SharedMap<int, int>::Dump(TEST_MAP_PATH, test_map);
        std::set<int> test_set = {1, 2, 3, 4, 5, 6, 7};
        SharedSet<int>::Dump(TEST_SET_PATH, test_set);
    }
    virtual void TearDown() {
        std::string cmd = "rm ";
        system((cmd + TEST_VEC_PATH).c_str());
        system((cmd + TEST_MAP_PATH).c_str());
        system((cmd + TEST_SET_PATH).c_str());
    }
};

TEST_F(SharedManagerTest, test_manager) {
    std::shared_ptr<SharedContainerManager> manager_ptr = NULL;
    manager_ptr.reset(new SharedContainerManager(TEST_GROUP_ID, TEST_APP_ID));

    //注册
    std::shared_ptr<SharedVector<int> > vec_ptr;
    std::shared_ptr<SharedMap<int, int> > map_ptr;
    std::shared_ptr<SharedSet<int> > set_ptr;
    EXPECT_EQ(manager_ptr->Register(TEST_VEC_PATH, vec_ptr), SC_RET_OK);
    EXPECT_EQ(manager_ptr->Register(TEST_MAP_PATH, map_ptr), SC_RET_OK);
    EXPECT_EQ(manager_ptr->Register(TEST_SET_PATH, set_ptr), SC_RET_OK);

    //查询
    std::shared_ptr<SharedVector<int> > vec_ptr_other;
    std::shared_ptr<SharedMap<int, int> > map_ptr_other;
    std::shared_ptr<SharedSet<int> > set_ptr_other;
    EXPECT_EQ(manager_ptr->GetContanerPtr(TEST_VEC_PATH, vec_ptr_other), SC_RET_OK);
    EXPECT_EQ(manager_ptr->GetContanerPtr(TEST_MAP_PATH, map_ptr_other), SC_RET_OK);
    EXPECT_EQ(manager_ptr->GetContanerPtr(TEST_SET_PATH, set_ptr_other), SC_RET_OK);

    //重复注册
    EXPECT_EQ(manager_ptr->Register(TEST_VEC_PATH, vec_ptr), SC_RET_HAS_REGISTED);
    EXPECT_EQ(manager_ptr->Register(TEST_MAP_PATH, map_ptr), SC_RET_HAS_REGISTED);

    //文件缺失
    std::shared_ptr<SharedVector<int> > vec_ptr_err;
    EXPECT_EQ(manager_ptr->Register("./err_path", vec_ptr_err), SC_RET_FILE_NOEXIST);

    //按文件清理
    manager_ptr.reset();
    vec_ptr.reset();
    map_ptr.reset();
    set_ptr.reset();
    vec_ptr_other.reset();
    map_ptr_other.reset();
    set_ptr_other.reset();
    std::set<std::string> reserve_files = {TEST_VEC_PATH, TEST_MAP_PATH};
    EXPECT_EQ(SharedContainerManager::ClearByFileList(reserve_files, TEST_APP_ID), SC_RET_OK);
    sleep(2);
    std::vector<SharedMidInfo> shmid_info;
    EXPECT_TRUE(SharedMemory::get_all_shmid(shmid_info, true));
    EXPECT_TRUE(shmid_info.size() != 0);
    for (size_t i = 0; i < shmid_info.size(); i++) {
        if (shmid_info[i].path == TEST_SET_PATH) {
            EXPECT_TRUE(false);
        }
    }

    //清理未注册的
    EXPECT_EQ(SharedContainerManager::ClearUnregistered(TEST_APP_ID), SC_RET_OK);
    sleep(2);
    EXPECT_TRUE(SharedMemory::get_all_shmid(shmid_info, true));
    EXPECT_TRUE(shmid_info.size() == 0);

    //按group清理
    manager_ptr.reset(new SharedContainerManager(TEST_GROUP_ID, TEST_APP_ID));
    EXPECT_EQ(manager_ptr->Register(TEST_VEC_PATH, vec_ptr), SC_RET_OK);
    EXPECT_EQ(manager_ptr->Register(TEST_MAP_PATH, map_ptr), SC_RET_OK);
    EXPECT_EQ(manager_ptr->Register(TEST_SET_PATH, set_ptr), SC_RET_OK);
    manager_ptr.reset();
    vec_ptr.reset();
    map_ptr.reset();
    set_ptr.reset();
    sleep(2);
    std::set<std::string> reserve_groups;
    EXPECT_EQ(SharedContainerManager::ClearByGroup(reserve_groups, TEST_APP_ID), SC_RET_OK);
    EXPECT_TRUE(SharedMemory::get_all_shmid(shmid_info, true));
    for (size_t i = 0; i < shmid_info.size(); i++) {
        std::cout << "------" << shmid_info[i].path << std::endl;
    }
    EXPECT_TRUE(shmid_info.size() == 0);

    //主动释放
    manager_ptr.reset(new SharedContainerManager(TEST_GROUP_ID, TEST_APP_ID));
    EXPECT_EQ(manager_ptr->Register(TEST_VEC_PATH, vec_ptr), SC_RET_OK);
    EXPECT_EQ(manager_ptr->Register(TEST_MAP_PATH, map_ptr), SC_RET_OK);
    EXPECT_EQ(manager_ptr->Register(TEST_SET_PATH, set_ptr), SC_RET_OK);
    manager_ptr->Release();
    manager_ptr.reset();
    vec_ptr.reset();
    map_ptr.reset();
    set_ptr.reset();
    sleep(2);
    EXPECT_TRUE(SharedMemory::get_all_shmid(shmid_info, true));
    EXPECT_TRUE(shmid_info.size() == 0);
}

} // namespace levin

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
