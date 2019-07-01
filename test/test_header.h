#ifndef __LEVIN_TEST_HEADER_H__
#define __LEVIN_TEST_HEADER_H__

#include <sys/types.h>
#include <gtest/gtest.h>
#include "xsi_shm.hpp"
#include "levin_logger.h"

namespace levin {

struct /*__attribute__((aligned(4)))*/ Cat {
    static const size_t CAT_NAME_SIZE = 12;
    char name[CAT_NAME_SIZE + 1];
    Cat() {
        memset(name, 0, sizeof(name));
    }
    Cat(const char *n) {
        strncpy(name, n, CAT_NAME_SIZE);
        name[CAT_NAME_SIZE] = '\0';
    }
};
struct CatComp {
    bool operator()(const Cat& lhs, const Cat& rhs) const {
        return strcmp(lhs.name, rhs.name) < 0;
    }
};
struct CatHash {
    size_t operator()(const Cat &cat) const {
        return std::hash<std::string>()(cat.name);
    }
};
struct CatEqual {
    bool operator()(const Cat& lhs, const Cat& rhs) const {
        return strcmp(lhs.name, rhs.name) == 0;
    }
};
inline std::ostream& operator<<(std::ostream &os, const Cat &cat) {
    return os << cat.name;
}

void CleanEnvShm(std::vector<std::string> names) {
    std::vector<SharedMidInfo> map_shmid;
    levin::SharedMemory::get_all_shmid(map_shmid);
    for (auto &shm : map_shmid) {
        auto kv = std::find(names.begin(), names.end(), shm.path);
        if (kv != names.end()) {
            levin::SharedMemory::remove_shared_memory(shm.mid);
            LEVIN_CDEBUG_LOG("CleanEnvShm %s,%d", shm.path.c_str(), shm.mid);
        }
    }
}

}  // namespace levin

#endif  // __LEVIN_TEST_HEADER_H__
