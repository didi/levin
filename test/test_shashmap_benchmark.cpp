#include <iostream>
#include <set>
#include <unordered_map>
#include "test_header.h"
#include "benchmark_header.h"
#include "shared_base.hpp"
#include "shared_utils.h"
#include "shared_allocator.h"
#include "smap.hpp"
#include "shashmap.hpp"
#include "snested_hashmap.hpp"

namespace levin {

static const size_t count = 1000000;
static const size_t col_count = 100;

void make_mapdata(std::vector<size_t> &vec_key) {
    srand((unsigned)time(NULL));
    vec_key.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        vec_key.emplace_back(rand() % (count-1));
    }
}

void dump_shared_map() {
    std::string name = "./smap_bench.dat";
    std::map<uint64_t, uint32_t> map_data;
    for (size_t i = 0; i < count; ++i) {
        map_data[i] = i + 100;
    }
    levin::SharedMap<uint64_t, uint32_t>::Dump(name, map_data);
    std::map<uint64_t, uint32_t>().swap(map_data);
}

void test_shared_map(const std::vector<size_t> &vec_key) {
    MemDiffer differ;
    std::string name = "./smap_bench.dat";
    levin::SharedMap<uint64_t, uint32_t> mymap(name);
    if (mymap.Init() != SC_RET_OK || mymap.Load() != SC_RET_OK) {
        std::cout << "init or load failed.";
        return;
    }
    std::cout << differ.get_diff_report_kb() << std::endl;

    levin::Timer rtimer;
    for (size_t i = 0; i < count; ++i) {
        auto &tmp = mymap[vec_key[i]];
        //std::cout << vec_key[i] << " map val:" << tmp << std::endl;
        if (tmp == count) {
            std::cout << tmp << std::endl;
        }
    }
    std::cout << "shm_map get time:" << rtimer.get_time_us() << std::endl;

    levin::Timer traversal_timer;
    for(auto item : mymap){
        auto key = item.first;
        auto value = item.second;
        if (key == count){
            std::cout << key << " ===== " << value << std::endl;
        }
    }
    std::cout << "shm_map traversal_time:" << traversal_timer.get_time_us() << std::endl;

    mymap.Destroy();
}

void dump_shared_hashmap() {
    std::string name = "./shashmap_bench.dat";
    std::unordered_map<uint64_t, uint32_t> map_data;
    for (size_t i = 0; i < count; ++i) {
        map_data[i] = i + 100;
    }
    levin::SharedHashMap<uint64_t, uint32_t>::Dump(name, map_data);
    std::unordered_map<uint64_t, uint32_t>().swap(map_data);
}

void test_shared_hashmap(const std::vector<size_t> &vec_key) {
    MemDiffer differ;
    std::string name = "./shashmap_bench.dat";
    levin::SharedHashMap<uint64_t, uint32_t> mymap(name);
    if (mymap.Init() != SC_RET_OK || mymap.Load() != SC_RET_OK) {
        std::cout << "init or load failed.";
        return;
    }
    std::cout << differ.get_diff_report_kb() << std::endl;

    levin::Timer rtimer;
    for (size_t i = 0; i < count; ++i) {
        auto &tmp = mymap[vec_key[i]];
        if (tmp == count){
            std::cout << tmp << std::endl;
        }
    }
    std::cout << "shm_hashmap get time:" << rtimer.get_time_us() << std::endl;

    levin::Timer traversal_timer;
    for(auto item : mymap){
        auto key = item.first;
        auto value = item.second;
        if (key == count){
            std::cout << key << " ===== " << value << std::endl;
        }
    }
    std::cout << "shm_hashmap traversal_time:" << traversal_timer.get_time_us() << std::endl;
    std::cout << "shm_hashmap bucket_count=" << mymap.bucket_size() << std::endl;

    mymap.Destroy();
}

void test_interprocess_map(const std::vector<size_t> &vec_key) {
    std::string name = "./boost_hashmap_bench.dat";
    std::ofstream fout(name, std::ios::out);
    fout.close();
    boost::interprocess::xsi_key key(name.c_str(), 77);
    boost::interprocess::managed_xsi_shared_memory shm(
            boost::interprocess::create_only, key, 1024 * 1024 * 1024);
    VoidAllocator alloc_instance(shm.get_segment_manager());
    Int64Int32Map *imap_ptr = shm.construct<Int64Int32Map>("Int64Int32Map")(alloc_instance);
    auto &mymap = *imap_ptr;
    for (size_t i = 0; i < count; ++i) {
        mymap[i] = i + 100;
    }
    std::cout << "used=" << (shm.get_size() - shm.get_free_memory()) / 1024 << "(kB)" << std::endl;

    levin::Timer rtimer;
    for (size_t i = 0; i < count; ++i) {
        auto &tmp = mymap[vec_key[i]];
        if (tmp == count){
            std::cout << tmp << std::endl;
        }
    }
    std::cout << "boost interprocess map get time:" << rtimer.get_time_us() << std::endl;

    levin::Timer traversal_timer;
    for (auto item : mymap) {
        auto key = item.first;
        auto value = item.second;
        if (key == count){
            std::cout << key << " ===== " << value << std::endl;
        }
    }
    std::cout << "boost interprocess map traversal_time:" << traversal_timer.get_time_us() << std::endl;

    boost::interprocess::xsi_shared_memory::remove(shm.get_shmid());
}

void test_std_map(const std::vector<size_t> &vec_key) {
    MallocDiffer differ;
    std::map<uint64_t, uint32_t> stdMap;
    for (size_t i = 0; i < count; ++i) {
        stdMap[i] = i + 100;
    }
    std::cout << differ.get_diff_report_kb() << std::endl;

    levin::Timer rtimer;
    for (size_t i = 0; i < count; ++i) {
        auto &tmp = stdMap[vec_key[i]];
        if(tmp == count){
            std::cout << tmp << std::endl;
        }
    }
    std::cout << "std map get time:" << rtimer.get_time_us() << std::endl;

    levin::Timer traversal_timer;
    for (auto item : stdMap) {
        auto key = item.first;
        auto value = item.second;
        if (key == count){
            std::cout << key << " ===== " << value << std::endl;
        }
    }
    std::cout << "std map traversal_time:" << traversal_timer.get_time_us() << std::endl;

    std::map<uint64_t, uint32_t>().swap(stdMap);
}

void test_std_hashmap(const std::vector<size_t> &vec_key) {
    MallocDiffer differ;
    std::unordered_map<uint64_t, uint32_t> stdHashMap;
    for (size_t i = 0; i < count; ++i) {
        stdHashMap[i] = i + 100;
    }
    std::cout << differ.get_diff_report_kb() << std::endl;

    levin::Timer rtimer;
    for (size_t i = 0; i < count; ++i) {
        auto &tmp = stdHashMap[vec_key[i]];
        if(tmp == count){
            std::cout << tmp << std::endl;
        }
    }
    std::cout << "std hashmap get time:" << rtimer.get_time_us() << std::endl;

    levin::Timer traversal_timer;
    for (auto item : stdHashMap) {
        auto key = item.first;
        auto value = item.second;
        if (key == count){
            std::cout << key << " ===== " << value << std::endl;
        }
    }
    std::cout << "std hashmap traversal_time:" << traversal_timer.get_time_us() << std::endl;

    std::cout << "std hashmap bucket_count=" << stdHashMap.bucket_count() << std::endl;
    std::unordered_map<uint64_t, uint32_t>().swap(stdHashMap);
}

void compare_map() {
    std::vector<size_t> vec_key;
    make_mapdata(vec_key);
    dump_shared_map();
    test_shared_map(vec_key);
    dump_shared_hashmap();
    test_shared_hashmap(vec_key);
    test_std_hashmap(vec_key);
    test_interprocess_map(vec_key);
    test_std_map(vec_key);
}

// ************************* nested map ************************* //
void make_nested_mapdata(std::vector<std::pair<size_t, size_t> > &vec_key) {
    srand((unsigned)time(NULL)); 
    for (size_t i = 0; i < count; ++i) {
        vec_key.emplace_back(std::make_pair(rand() % (count - 1), rand() % col_count + 1));
    }
}

void dump_nested_map(const std::vector<std::pair<size_t, size_t> > &vec_key) {
    std::string name = "./snhashmap.dat";
    std::unordered_map<uint64_t, std::vector<uint32_t> > nmap_data;
    for (size_t i = 0; i < count; ++i) {
        std::vector<uint32_t> vec(vec_key[i].second, i);
        nmap_data[i].assign(vec.begin(), vec.end());
    }
    levin::SharedNestedHashMap<uint64_t, uint32_t>::Dump(name, nmap_data);
}

void test_shared_nested_map(const std::vector<std::pair<size_t, size_t> > &vec_key) {
    std::string name = "./snhashmap.dat";
    levin::SharedNestedHashMap<uint64_t, uint32_t>  mymap(name);
    if (mymap.Init() != SC_RET_OK || mymap.Load() != SC_RET_OK) {
        std::cout << "load file false." << std::endl;
        return;
    }

    levin::Timer rtimer;
    for (size_t i = 0; i < count; ++i) {
        auto tmp = mymap[vec_key[i].first];
        if (tmp->empty()){
            std::cout << vec_key[i].first << " shm " << tmp->size() << std::endl;
        }
    }
    std::cout << "shm nested map get time:" << rtimer.get_time_us() << std::endl;

    levin::Timer traversal_timer;
    for (auto item : mymap) {
        auto key = item.first;
        auto value = item.second;
        if (key == count){
            std::cout << key << " ===== " << value->size() << std::endl;
        }
    }
    std::cout << "shm nested map traversal_time:" << traversal_timer.get_time_us() << std::endl;

    MemDiffer differ;
    mymap.Destroy();
    std::cout << differ.get_diff_report_kb() << std::endl;
}

void test_std_nested_map(const std::vector<std::pair<size_t, size_t> > &vec_key) {
    MallocDiffer differ;
    std::map<uint64_t, std::vector<uint32_t> > stdMap;
    for (size_t i = 0; i < count; ++i) {
        std::vector<uint32_t> vec(vec_key[i].second, i);
        stdMap[i].assign(vec.begin(), vec.end());
    }
    std::cout << differ.get_diff_report_kb() << std::endl;

    levin::Timer rtimer;
    for (size_t i = 0; i < count; ++i) {
        auto &sec = stdMap[vec_key[i].first];
        if (sec.empty()) {
            std::cout << vec_key[i].first << " std " << sec.size() << std::endl;
        }
    }
    std::cout << "std nested map get time:" << rtimer.get_time_us() << std::endl;

    levin::Timer traversal_timer;
    for (auto item : stdMap) {
        auto key = item.first;
        auto value = item.second;
        if (key == count) {
            std::cout << key << " ===== " << value.size() << std::endl;
        }
    }
    std::cout << "std nested map traversal_time:" << traversal_timer.get_time_us() << std::endl;
}

void test_std_nested_hashmap(const std::vector<std::pair<size_t, size_t> > &vec_key) {
    MallocDiffer differ;
    std::unordered_map<uint64_t, std::vector<uint32_t> > stdMap;
    for (size_t i = 0; i < count; ++i) {
        std::vector<uint32_t> vec(vec_key[i].second, i);
        stdMap[i].assign(vec.begin(), vec.end());
    }
    std::cout << differ.get_diff_report_kb() << std::endl;

    levin::Timer rtimer;
    for (size_t i = 0; i < count; ++i) {
        auto &sec = stdMap[vec_key[i].first];
        if (sec.empty()) {
            std::cout << vec_key[i].first << " std " <<sec.size() << std::endl;
        }
    }
    std::cout << "std nested hashmap get time:" << rtimer.get_time_us() << std::endl;

    levin::Timer traversal_timer;
    for (auto item : stdMap) {
        auto key = item.first;
        auto value = item.second;
        if (key == count) {
            std::cout << key << " ===== " << value.size() << std::endl;
        }
    }
    std::cout << "std nested hashmap traversal_time:" << traversal_timer.get_time_us() << std::endl;
}

void compare_nested_map() {
    std::vector<std::pair<size_t, size_t> > vec_key;
    make_nested_mapdata(vec_key);
    dump_nested_map(vec_key);

    test_shared_nested_map(vec_key);
    test_std_nested_map(vec_key);
    test_std_nested_hashmap(vec_key);
}

}  // namespace levin

int main() {
    levin::compare_map();
    std::cout << "---------------------" << std::endl;
    levin::compare_nested_map();

    return 0;
}
