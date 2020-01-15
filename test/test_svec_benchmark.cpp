#include <iostream>
#include <time.h>
#include "test_header.h"
#include "benchmark_header.h"
#include "svec.hpp"

namespace levin {

static const size_t COUNT = 10000000;
static const size_t ROW_COUNT = 1000000;
static const size_t COL_COUNT = 10;
static const int    RUN_TIMES = 1;

void gen_access_idxs(std::vector<size_t> &access_idxs) {
    srand(time(nullptr));
    // for random set/get
    for (size_t i = 0; i < COUNT; ++i) {
        access_idxs.emplace_back(rand() % COUNT);
    }
}

void test_shared_vector(const std::vector<size_t> &access_idxs) {
    std::vector<int32_t> in;
    in.resize(COUNT, 0);
    std::string name = "./svec_bench.dat";
    levin::SharedVector<int32_t>::Dump(name, in);

    levin::SharedVector<int32_t> vec(name);
    if (vec.Init() != SC_RET_OK || (!vec.IsExist() && vec.Load() != SC_RET_OK)) {
        return;
    }
    // DIFF
    EXPECT_EQ(in.size(), vec.size());
    for (auto idx : access_idxs) {
        EXPECT_EQ(in[idx], vec[idx]);
    }
    std::vector<int32_t>().swap(in);

    levin::Timer wtimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            vec[access_idxs[i]] = i;
        }
    }
    std::cout << "SharedVector random set time:" << wtimer.get_time_us() << std::endl;

    levin::Timer rtimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            const auto &tmp = vec[access_idxs[i]];
            if (tmp < 0) {
                std::cout << tmp << std::endl;
            }
        }
    }
    std::cout << "SharedVector random get time:" << rtimer.get_time_us() << std::endl;
    levin::Timer ttimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (auto &elem : vec) {
            if (elem < 0) { std::cout << elem << std::endl; }
        }
    }
    std::cout << "SharedVector traversal time:" << ttimer.get_time_us() << std::endl;
    LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());

    MemDiffer differ;
    vec.Destroy();
    std::cout << differ.get_diff_report_kb() << std::endl;
}

void test_interprocess_vector(const std::vector<size_t> &access_idxs) {
    std::string name = "./boost_vec_bench.dat";
    std::ofstream fout(name, std::ios::out);
    fout.close();
    boost::interprocess::xsi_key key(name.c_str(), 77);
    boost::interprocess::managed_xsi_shared_memory shm(
            boost::interprocess::create_only, key, 1024 * 1024 * 200);
    VoidAllocator alloc_instance(shm.get_segment_manager());
    IntVector *ivec_ptr = shm.construct<IntVector>("IntVector")(alloc_instance);
    auto &vec = *ivec_ptr;
    vec.resize(COUNT, 0);
    std::cout << "used=" << (shm.get_size() - shm.get_free_memory()) / 1024 << "(kB)" << std::endl;

    levin::Timer wtimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            vec[access_idxs[i]] = i;
        }
    }
    std::cout << "boost interprocess vector random set time:" << wtimer.get_time_us() << std::endl;

    levin::Timer rtimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            const auto &tmp = vec[access_idxs[i]];
            if (tmp < 0) {
                std::cout << tmp << std::endl;
            }
        }
    }
    std::cout << "boost interprocess vector random get time:" << rtimer.get_time_us() << std::endl;
    levin::Timer ttimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (auto &elem : vec) {
            if (elem < 0) { std::cout << elem << std::endl; }
        }
    }
    std::cout << "boost interprocess vector traversal time:" << ttimer.get_time_us() << std::endl;

    boost::interprocess::xsi_shared_memory::remove(shm.get_shmid());
}

void test_std_vector(const std::vector<size_t> &access_idxs) {
    std::vector<int32_t> vec;
    vec.resize(COUNT, 0);

    levin::Timer wtimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            vec[access_idxs[i]] = i;
        }
    }
    std::cout << "std vector random set time:" << wtimer.get_time_us() << std::endl;

    levin::Timer rtimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            const auto &tmp = vec[access_idxs[i]];
            //int tmp = in.at(access_idxs[i]);
            if (tmp < 0) {
                std::cout << tmp << std::endl;
            }
        }
    }
    std::cout << "std vector random get time:" << rtimer.get_time_us() << std::endl;
    levin::Timer ttimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (auto &elem : vec) {
            if (elem < 0) { std::cout << elem << std::endl; }
        }
    }
    std::cout << "std vector traversal time:" << ttimer.get_time_us() << std::endl;

    MallocDiffer mdiffer;
    std::vector<int32_t>().swap(vec);
    std::cout << mdiffer.get_diff_report_kb() << std::endl;
}

void vector_benchmark() {
    std::vector<size_t> access_idxs;
    gen_access_idxs(access_idxs);
    test_interprocess_vector(access_idxs);
    std::cout << std::endl;
    test_shared_vector(access_idxs);
    std::cout << std::endl;
    test_std_vector(access_idxs);
}

// ************************* nested vector ************************* //
void gen_access_idxs(std::vector<std::pair<size_t, size_t> > &access_idxs) {
    srand(time(nullptr));
    // for random set/get
    for (size_t i = 0; i < COUNT; ++i) {
        access_idxs.emplace_back(std::make_pair(rand() % ROW_COUNT, rand() % COL_COUNT));
    }
}

void gen_nested_vector(
        const std::vector<std::pair<size_t, size_t> > &access_idxs,
        std::vector<std::vector<int> > &in) {
    in.resize(ROW_COUNT, std::vector<int>());
    for (size_t i = 0; i < ROW_COUNT; ++i) {
        in[i].resize(COL_COUNT, 0);
    }
    for (size_t i = 0; i < COUNT; ++i) {
        in[access_idxs[i].first][access_idxs[i].second] = i;
    }
}

void test_shared_nested_vector(const std::vector<std::pair<size_t, size_t> > &access_idxs) {
    std::vector<std::vector<int> > in;
    gen_nested_vector(access_idxs, in);
    std::string name = "./nsvec_bench.dat";
    levin::SharedNestedVector<int>::Dump(name.c_str(), in);

    levin::SharedNestedVector<int> vec(name);
    if (vec.Init() != SC_RET_OK || (!vec.IsExist() && vec.Load() != SC_RET_OK)) {
        return;
    }
    // DIFF
    ASSERT_TRUE(vec.size() == ROW_COUNT);
    for (auto &row : vec) {
        ASSERT_TRUE(row.size() == COL_COUNT);
    }
    for (auto &pair : access_idxs) {
        EXPECT_EQ(in[pair.first][pair.second], vec[pair.first][pair.second]);
    }
    std::vector<std::vector<int> >().swap(in);

    levin::Timer wtimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            vec[access_idxs[i].first][access_idxs[i].second] = i + 1;
        }
    }
    std::cout << "SharedNestedVector random set time:" << wtimer.get_time_us() << std::endl;
    levin::Timer rtimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            const int &tmp = vec[access_idxs[i].first][access_idxs[i].second];
            if (tmp < 0) {
                std::cout << tmp << std::endl;
            }
        }
    }
    std::cout << "SharedNestedVector random get time:" << rtimer.get_time_us() << std::endl;
    levin::Timer ttimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (auto &row : vec) {
            for (auto &elem : row) {
                if (elem < 0) { std::cout << elem << std::endl; }
            }
        }
    }
    std::cout << "SharedNestedVector traversal time:" << ttimer.get_time_us() << std::endl;
    LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());

    MemDiffer differ;
    vec.Destroy();
    std::cout << differ.get_diff_report_kb() << std::endl;
}

void test_std_nested_vector(const std::vector<std::pair<size_t, size_t> > &access_idxs) {
    MallocDiffer differ;
    std::vector<std::vector<int> > in;
    gen_nested_vector(access_idxs, in);
    std::cout << differ.get_diff_report_kb() << std::endl;

    levin::Timer wtimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            in[access_idxs[i].first][access_idxs[i].second] = i + 1;
        }
    }
    std::cout << "std nested vector random set time:" << wtimer.get_time_us() << std::endl;
    levin::Timer rtimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (size_t i = 0; i < COUNT; ++i) {
            int &tmp = in[access_idxs[i].first][access_idxs[i].second];
            if (tmp < 0) {
                std::cout << tmp << std::endl;
            }
        }
    }
    std::cout << "std nested vector random get time:" << rtimer.get_time_us() << std::endl;
    levin::Timer ttimer;
    for (int times = 0; times < RUN_TIMES; ++times) {
        for (auto &row : in) {
            for (auto &elem : row) {
                if (elem < 0) { std::cout << elem << std::endl; }
            }
        }
    }
    std::cout << "std nested vector traversal time:" << ttimer.get_time_us() << std::endl;

    std::vector<std::vector<int> >().swap(in);
}

void nested_vector_benchmark() {
    std::vector<std::pair<size_t, size_t> > access_idxs;
    gen_access_idxs(access_idxs);
    test_shared_nested_vector(access_idxs);
    std::cout << std::endl;
    test_std_nested_vector(access_idxs);
}

}  // namespace levin

int main() {
    levin::vector_benchmark();
    std::cout << "------------------------------------------" << std::endl;
    levin::nested_vector_benchmark();

    return 0;
}
