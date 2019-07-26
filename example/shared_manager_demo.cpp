/*
* we highly recommand you to use SharedContainerManager to manage all of your shared containers,
* it makes creating, using and releasing shared containers easier. 
* one SharedContainerManager object corresponds to one group identified by group ID.
* you should put shared containers which has same lifecycle in one group, so you can release them at once 
*/

#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <stdint.h>
#include "shared_manager.h"
#include "shared_utils.h"
#include "svec.hpp"
#include "smap.hpp"
#include "check_file.h"
#include "levin_logger.h"

using namespace levin;

#define GROUP_ID            "test_group_id"
#define APP_ID              1
#define VEC_DATA_PATH       "./test_vec"
#define MAP_DATA_PATH       "./test_map"
#define VEC_DATA_CHECK_MSG  "vec_file_md5"
#define MAP_DATA_CHECK_MSG  "map_file_md5"


bool mock_check_func(const std::string file_path, const std::string verify_msg) {
    if (verify_msg == MAP_DATA_CHECK_MSG || verify_msg == VEC_DATA_CHECK_MSG) {
        return true;
    }
    return false;
}

bool dump_test_data() {
    std::vector<int> test_vec = {1, 2, 3, 4, 5};
    if(!SharedVector<int>::Dump(VEC_DATA_PATH, test_vec)) {
        return false;
    }
    std::map<int, int> test_map = {{1,10}, {2,20}, {3,30}};
    if(!SharedMap<int, int>::Dump(MAP_DATA_PATH, test_map)) {
        return false;
    }
    return true;
}

int main() {
    int ret;

    if (!dump_test_data()) {
        return -1;
    }

    // (optional) three methods can be used to clear shared containers
    // which exist but not be using（release shared memory）
    {
        // 1 clear all shared containers except 'reserve_files'
        std::set<std::string> reserve_files = {VEC_DATA_PATH};
        ret = SharedContainerManager::ClearByFileList(reserve_files, APP_ID);
        if (SC_RET_OK != ret) {
            return -1;
        }
 
        // 2 clear all shareds containers except 'reserve_groups'
        std::set<std::string> reserve_groups = {GROUP_ID};
        ret = SharedContainerManager::ClearByGroup(reserve_groups, APP_ID);
        if (SC_RET_OK != ret) {
            return -1;
        }
 
        // 3 clear all shareds containers except those which has registered
        ret = SharedContainerManager::ClearUnregistered(APP_ID);
        if (SC_RET_OK != ret) {
            return -1;
        }
    }

    // create a SharedContainerManager object
    std::shared_ptr<SharedContainerManager> manager_ptr = NULL;
    manager_ptr.reset(new SharedContainerManager(GROUP_ID, APP_ID));

    // (optinal) set check function to guarantee the files loaded are correct
    std::map<std::string, std::string> verify_data = {
        {VEC_DATA_PATH, VEC_DATA_CHECK_MSG},
        {MAP_DATA_PATH, MAP_DATA_CHECK_MSG}
    };
    ret = SharedContainerManager::VerifyFiles(verify_data, mock_check_func, APP_ID);
    if (SC_RET_OK != ret) {
        return -1;
    }

    // register shared containers to group(involves loading data, checking file correction, recording info)
    std::shared_ptr<SharedVector<int> > vec_ptr;
    std::shared_ptr<SharedMap<int, int> > map_ptr;
    ret = manager_ptr->Register(VEC_DATA_PATH, vec_ptr);
    CHECK_RET(ret);
    ret = manager_ptr->Register(MAP_DATA_PATH, map_ptr);
    CHECK_RET(ret);

    // get a shared contaner's pointer
    std::shared_ptr<SharedVector<int> > other_vec_ptr = NULL;
    ret = SharedContainerManager::GetContanerPtr(VEC_DATA_PATH, other_vec_ptr);
    CHECK_RET(ret);
 
    // use shared container
    vec_ptr->size();
    other_vec_ptr->begin();
    map_ptr->find(0);
    // ...

    // (optional) release all shared containers in certain group when these shared containers are not needed anymore.
    // Attention: shared containers won't be destroyed immediately. levin scans and destroys containers once per second,
    //            and container won't be destroyed if it still being used
    manager_ptr->Release();
 
    return 0;
}
