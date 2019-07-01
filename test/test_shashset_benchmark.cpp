#include <iostream>
#include <time.h>
#include "test_header.h"
#include "benchmark_header.h"
#include "shashset.hpp"

namespace levin {

static const size_t COUNT = 1000000;
static const int    RUN_TIMES = 1;

void gen_keys(std::vector<int32_t> &access_keys) {
    srand(time(nullptr));
    // for random set/get
    for (size_t i = 0; i < COUNT; ++i) {
        access_keys.emplace_back(rand() % COUNT);
    }
}

void test_shared_hashset(const std::vector<int32_t> &keys, const std::vector<int32_t> &access_keys) {
    std::unordered_set<int32_t> in(keys.begin(), keys.end());
    std::string name = "./shashset_bench.dat";
    levin::SharedHashSet<int32_t>::Dump(name, in);

    levin::SharedHashSet<int32_t> hst(name);
    if (hst.Init() != SC_RET_OK || (!hst.IsExist() && hst.Load() != SC_RET_OK)) {
        return;
    }
    // DIFF
    EXPECT_EQ(in.size(), hst.size());
    for (const auto &key : access_keys) {
        EXPECT_EQ(in.count(key), hst.count(key));
    }
    std::unordered_set<int32_t>().swap(in);

    levin::Timer rtimer;
    int hit = 0;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            auto ret = hst.find(access_keys[i]);
            if (ret != hst.end()) {
                ++hit;
            }
        }
    }
    std::cout << "SharedHashSet set hit:" << hit << std::endl;
    std::cout << "SharedHashSet find time:" << rtimer.get_time_us() << std::endl;
    levin::Timer ttimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (auto &elem : hst) {
            if (elem < 0) { std::cout << elem << std::endl; }
        }
    }
    std::cout << "SharedHashSet traversal time:" << ttimer.get_time_us() << std::endl;

    MemDiffer differ;
    hst.Destroy();
    std::cout << differ.get_diff_report_kb() << std::endl;
}

void test_std_unordered_set(const std::vector<int32_t> &keys, const std::vector<int32_t> &access_keys) {
    std::unordered_set<int32_t> st(keys.begin(), keys.end());

    levin::Timer rtimer;
    int hit = 0;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            auto ret = st.find(access_keys[i]);
//            if (ret != st.end() && *ret < 0) {
//                std::cout << *ret << std::endl;
            if (ret != st.end()) {
                ++hit;
            }
        }
    }
    std::cout << "std unordered_set hit:" << hit << std::endl;
    std::cout << "std unordered_set find time:" << rtimer.get_time_us() << std::endl;
    levin::Timer ttimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (auto &elem : st) {
            if (elem < 0) { std::cout << elem << std::endl; }
        }
    }
    std::cout << "std unordered_set traversal time:" << ttimer.get_time_us() << std::endl;

    MallocDiffer mdiffer;
    std::unordered_set<int32_t>().swap(st);
    std::cout << mdiffer.get_diff_report_kb() << std::endl;
}

void hashset_benchmark() {
    std::vector<int32_t> keys;
    std::vector<int32_t> access_keys;
    gen_keys(keys);
    sleep(1);
    gen_keys(access_keys);
    test_shared_hashset(keys, access_keys);
    std::cout << std::endl;
    test_std_unordered_set(keys, access_keys);
}

}  // namespace levin

int main() {
    levin::hashset_benchmark();
    std::cout << "------------------------------------------" << std::endl;

    return 0;
}
