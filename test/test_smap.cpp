#include "smap.hpp"
#include <vector>
#include <map>
#include <unordered_map>
#include <gtest/gtest.h>
#include "test_header.h"

namespace levin {

class SharedMapTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
    const size_t fixed_len = SharedAllocator::Allocsize(sizeof(SharedMeta));
};

const size_t count = 10;
const size_t key_range = count + 500;
const size_t val_range = 10;

// 输入数据
std::vector<std::pair<size_t, size_t> > vec_key64;
std::map<uint64_t, uint64_t> map_kv64;

std::vector<std::pair<uint32_t, uint32_t> > vec_key32;
std::map<uint32_t, uint32_t> map_kv32;

std::unordered_map<size_t, size_t> map_filter;

void make_hashmapdata() {
    srand((unsigned)time(NULL));
    for (size_t i = 0; i < count;) {
        auto key = rand() % key_range;
        if (map_filter.count(key) == 0){
            vec_key64.emplace_back(std::make_pair(key, rand() % val_range + 1));
            vec_key32.emplace_back(std::make_pair(key, rand() % val_range + 1));
            map_filter[key] = 1;
            i++;
        }
    }
    for (size_t i = 0; i < count; ++i) {
        map_kv64[vec_key64[i].first] = vec_key64[i].second;
        map_kv32[vec_key32[i].first] = vec_key32[i].second;
    }
    return;
}

TEST_F(SharedMapTest, test_static_Dump64) {
    std::string name = "./smap64.dat";
    bool res = levin::SharedMap<uint64_t, uint64_t>::Dump(name, map_kv64);
    EXPECT_TRUE(res);
}

TEST_F(SharedMapTest, test_static_Dump32) {
    std::string name = "./smap32.dat";
    bool res = levin::SharedMap<uint32_t, uint32_t>::Dump(name, map_kv32);
    EXPECT_TRUE(res);
}

// 接口功能
TEST_F(SharedMapTest, test_Load64) {
    std::string name = "./smap64.dat";
    {
        levin::SharedMap<uint64_t, uint64_t> mymap(name);
        ASSERT_EQ(mymap.Init(), SC_RET_OK);
        ASSERT_EQ(mymap.Load(), SC_RET_OK);

        EXPECT_EQ(mymap.size(), map_kv64.size());
        EXPECT_FALSE(mymap.empty());
        uint64_t idx = rand() % count;
        auto key = vec_key64[idx].first;
        EXPECT_EQ(mymap.count(key), 1);
        EXPECT_EQ(mymap.at(key),  vec_key64[idx].second);
        auto it = mymap.find(key);
        EXPECT_EQ(it->second, vec_key64[idx].second);
        std::sort(vec_key64.begin(), vec_key64.end());

        key = vec_key64[idx].first;
        auto item = mymap.lower_bound(key);
        EXPECT_EQ(item->second, vec_key64[idx].second);

        item = mymap.upper_bound(key);
        if (item != mymap.end()){
            LEVIN_CDEBUG_LOG("upper_bound, key=%lu, val=%lu", item->first, item->second);
        }
        auto np = mymap.equal_range(key);
        LEVIN_CDEBUG_LOG("lower_bound: %lu => %lu", np.first->first, np.first->second);
        LEVIN_CDEBUG_LOG("lower_bound: %lu => %lu", np.second->first, np.second->second);
        for(auto bit = mymap.begin(); bit !=mymap.end(); ++bit){
            LEVIN_CDEBUG_LOG("%lu=====%lu", bit->first, bit->second);
        }
        LEVIN_CDEBUG_LOG("%s", mymap.layout().c_str());
        mymap.Destroy();
    }
}
// 接口功能
TEST_F(SharedMapTest, test_Load32) {
    std::string name = "./smap32.dat";
    {
        levin::SharedMap<uint32_t, uint32_t> mymap(name);
        ASSERT_EQ(mymap.Init(), SC_RET_OK);
        ASSERT_EQ(mymap.Load(), SC_RET_OK);

        EXPECT_EQ(mymap.size(), map_kv64.size());
        EXPECT_FALSE(mymap.empty());
        uint32_t idx = rand() % count;
        auto key = vec_key32[idx].first;
        EXPECT_EQ(mymap.count(key), 1);
        EXPECT_EQ(mymap.at(key),  vec_key32[idx].second);
        auto it = mymap.find(key);
        EXPECT_EQ(it->second, vec_key32[idx].second);

        std::sort(vec_key32.begin(), vec_key32.end());
        key = vec_key32[idx].first;
        auto item = mymap.lower_bound(key);
        EXPECT_EQ(item->second, vec_key32[idx].second);

        item = mymap.upper_bound(key);
        if (item != mymap.end()){
            LEVIN_CDEBUG_LOG("upper_bound, key=%u, val=%u", item->first, item->second);
        }
        auto np = mymap.equal_range(key);
        LEVIN_CDEBUG_LOG("lower_bound: %u => %u", np.first->first, np.first->second);
        LEVIN_CDEBUG_LOG("lower_bound: %u => %u", np.second->first, np.second->second);

        for(auto bit = mymap.begin(); bit !=mymap.end(); ++bit){
            LEVIN_CDEBUG_LOG("%u=====%u", bit->first, bit->second);
        }
        LEVIN_CDEBUG_LOG("%s", mymap.layout().c_str());
        mymap.Destroy();
    }
}

//校验文件写入共享内存大小与load后共享内存大小一致性
TEST_F(SharedMapTest, test_check_map_memsize) {
    std::string name = "./smap32.dat";
    std::ifstream fin(name, std::ios::in | std::ios::binary);
    if (!fin.is_open()) {
        LEVIN_CWARNING_LOG("opend file fail:%s", name.c_str());
        return;
    }
    size_t mem_size = 0;
    fin.read((char*)&mem_size, sizeof(mem_size));
    fin.close();
    EXPECT_TRUE(mem_size > 0);
    {
        levin::SharedMap<uint32_t, uint32_t> mymap(name);
        ASSERT_EQ(mymap.Init(), SC_RET_OK);
        ASSERT_EQ(mymap.Load(), SC_RET_OK);

        size_t load_size = mymap.load_shm_size();
        LEVIN_CDEBUG_LOG("file mem_size:%lu, load mem_size:%lu", mem_size, load_size);
        EXPECT_EQ(mem_size + fixed_len, load_size);

        mymap.Destroy();
    }
}

TEST_F(SharedMapTest, test_smap_diff) {
    srand((unsigned)time(NULL));

    std::string name = "./smapdiff.dat";
    std::map<uint64_t, uint32_t> stdmap;
    for (size_t i = 0; i < count; ++i) {
        stdmap[vec_key32[i].first] = vec_key32[i].second;
    }
    bool res = levin::SharedMap<uint64_t, uint32_t>::Dump(name, stdmap);
    EXPECT_TRUE(res);

    //load smap
    levin::SharedMap<uint64_t, uint32_t> mymap(name);
    ASSERT_EQ(mymap.Init(), SC_RET_OK);
    ASSERT_EQ(mymap.Load(), SC_RET_OK);

    //diff value
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(stdmap[vec_key32[i].first], mymap[vec_key32[i].first]);
    }
    mymap.Destroy();
}

}  // namespace levin

int main(int argc, char** argv) {
    //mock data
    levin::make_hashmapdata();

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
