#include <iostream>
#include <time.h>
#include "test_header.h"
#include "benchmark_header.h"
#include "sset.hpp"

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

void test_shared_set(const std::vector<int32_t> &keys, const std::vector<int32_t> &access_keys) {
    std::set<int32_t> in(keys.begin(), keys.end());
    std::string name = "./sset_bench.dat";
    levin::SharedSet<int32_t>::Dump(name, in);

    levin::SharedSet<int32_t> st(name);
    if (st.Init() != SC_RET_OK || (!st.IsExist() && st.Load() != SC_RET_OK)) {
        return;
    }
    // DIFF
    EXPECT_EQ(in.size(), st.size());
    for (const auto &key : access_keys) {
        EXPECT_EQ(in.count(key), st.count(key));
    }
    std::set<int32_t>().swap(in);

    levin::Timer rtimer;
    int hit = 0;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            auto ret = st.find(access_keys[i]);
            if (ret != st.end()) {
                ++hit;
            }
        }
    }
    std::cout << "SharedSet hit:" << hit << std::endl;
    std::cout << "SharedSet find time:" << rtimer.get_time_us() << std::endl;
    levin::Timer ctimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            auto ret = st.lower_bound(access_keys[i]);
            if (ret != st.end() &&  *ret < 0) {
                std::cout << *ret << std::endl;
            }
        }
    }
    std::cout << "SharedSet range compare time:" << ctimer.get_time_us() << std::endl;
    levin::Timer ttimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (auto &elem : st) {
            if (elem < 0) { std::cout << elem << std::endl; }
        }
    }
    std::cout << "SharedSet traversal time:" << ttimer.get_time_us() << std::endl;

    MemDiffer differ;
    st.Destroy();
    std::cout << differ.get_diff_report_kb() << std::endl;
}

void test_interprocess_set(const std::vector<int32_t> &keys, const std::vector<int32_t> &access_keys) {
    std::string name = "./boost_set_bench.dat";
    std::ofstream fout(name, std::ios::out);
    fout.close();
    boost::interprocess::xsi_key key(name.c_str(), 77);
    boost::interprocess::managed_xsi_shared_memory shm(
            boost::interprocess::create_only, key, 1024 * 1024 * 200);
    VoidAllocator alloc_instance(shm.get_segment_manager());
    IntSet *iset_ptr = shm.construct<IntSet>("IntSet")(alloc_instance);
    auto &st = *iset_ptr;
    for (auto &elem : keys) {
        st.emplace(elem);
    }
    std::cout << "used=" << (shm.get_size() - shm.get_free_memory()) / 1024 << "(kB)" << std::endl;

    levin::Timer rtimer;
    int hit = 0;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            auto ret = st.find(access_keys[i]);
            if (ret != st.end()) {
                ++hit;
            }
        }
    }
    std::cout << "boost interprocess set hit:" << hit << std::endl;
    std::cout << "boost interprocess set find time:" << rtimer.get_time_us() << std::endl;
    levin::Timer ctimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            auto ret = st.lower_bound(access_keys[i]);
            if (ret != st.end() &&  *ret < 0) {
                std::cout << *ret << std::endl;
            }
        }
    }
    std::cout << "boost interprocess set range compare time:" << ctimer.get_time_us() << std::endl;
    levin::Timer ttimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (auto &elem : st) {
            if (elem < 0) { std::cout << elem << std::endl; }
        }
    }
    std::cout << "boost interprocess set traversal time:" << ttimer.get_time_us() << std::endl;

    boost::interprocess::xsi_shared_memory::remove(shm.get_shmid());
}

void test_std_set(const std::vector<int32_t> &keys, const std::vector<int32_t> &access_keys) {
    std::set<int32_t> st(keys.begin(), keys.end());

    levin::Timer rtimer;
    int hit = 0;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            auto ret = st.find(access_keys[i]);
            if (ret != st.end()) {
                ++hit;
            }
        }
    }
    std::cout << "std set hit:" << hit << std::endl;
    std::cout << "std set find time:" << rtimer.get_time_us() << std::endl;
    levin::Timer ctimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            auto ret = st.lower_bound(access_keys[i]);
            if (ret != st.end() &&  *ret < 0) {
                std::cout << *ret << std::endl;
            }
        }
    }
    std::cout << "std set range compare time:" << ctimer.get_time_us() << std::endl;
    levin::Timer ttimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (auto &elem : st) {
            if (elem < 0) { std::cout << elem << std::endl; }
        }
    }
    std::cout << "std set traversal time:" << ttimer.get_time_us() << std::endl;

    MallocDiffer mdiffer;
    std::set<int32_t>().swap(st);
    std::cout << mdiffer.get_diff_report_kb() << std::endl;
}

void set_benchmark() {
    std::vector<int32_t> keys;
    std::vector<int32_t> access_keys;
    gen_keys(keys);
    sleep(1);
    gen_keys(access_keys);
    test_interprocess_set(keys, access_keys);
    std::cout << std::endl;
    test_shared_set(keys, access_keys);
    std::cout << std::endl;
    test_std_set(keys, access_keys);
}

}  // namespace levin

int main() {
    levin::set_benchmark();
    std::cout << "------------------------------------------" << std::endl;

    return 0;
}
