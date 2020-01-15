#include "snested_hashmap.hpp"
#include "shashmap.hpp"
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
    const size_t fixed_len = SharedBase::MetaSize() + SharedBase::HeaderSize();
};

const size_t count = 100;
const size_t key_range = 500;
const size_t nested_count = 10; //嵌套vector最大个数

// 输入数据
std::vector<std::pair<size_t, size_t> > vec_key64;
std::map<uint64_t, uint64_t> map_kv64;

std::vector<std::pair<uint32_t, uint32_t> > vec_key32;
std::map<uint32_t, uint32_t> map_kv32;

std::map<uint32_t, std::vector<uint32_t> > nmap_data;
std::unordered_map<size_t, size_t> map_filter;

void make_mapdata() {
    srand((unsigned)time(NULL));
    for (size_t i = 0; i < count;) {
        auto key = rand() % key_range;
        if (map_filter.count(key) == 0){
            vec_key64.emplace_back(std::make_pair(key, rand() % nested_count + 1));
            vec_key32.emplace_back(std::make_pair(key, rand() % nested_count + 1));
            map_filter[key] = 1;
            i++;
        }
    }
    for (size_t i = 0; i < count; ++i) {
        map_kv64[vec_key64[i].first] = vec_key64[i].second;
        map_kv32[vec_key32[i].first] = vec_key32[i].second;

        std::vector<uint32_t> vec(vec_key32[i].second, i);
        nmap_data[i].assign(vec.begin(), vec.end());
    }
    return;
}

TEST_F(SharedMapTest, test_type_traits_assert) {
    // error type, compile error is expected
//    levin::SharedHashMap<int, uint64_t>::Dump("", map_kv64);
//    levin::SharedHashMap<uint64_t, int64_t>::Dump("", map_kv64);
//    std::unordered_map<uint64_t, uint64_t> hashmap_kv64;
//    levin::SharedHashMap<int, uint64_t>::Dump("", hashmap_kv64);
//    levin::SharedHashMap<uint64_t, int64_t>::Dump("", hashmap_kv64);
}

TEST_F(SharedMapTest, test_static_Dump_Load_empty) {
    std::string name = "./hashmap_empty.dat";
    // dump
    {
        std::unordered_map<int, int> in;
        bool succ = levin::SharedHashMap<int, int>::Dump(name, in);
        EXPECT_TRUE(succ);
    }
    // load
    {
        levin::SharedHashMap<int, int> smap(name);
        bool succ = (smap.Init() == SC_RET_OK && (smap.IsExist() || smap.Load() == SC_RET_OK));
        ASSERT_TRUE(succ);
        LEVIN_CDEBUG_LOG("%s", smap.layout().c_str());
        EXPECT_TRUE(smap.empty());
        EXPECT_EQ(smap.size(), 0);
        EXPECT_TRUE(smap.find(1) == smap.end());
        for (auto it = smap.begin(); it != smap.end(); ++it) {
            std::cout << it->first << "," << it->second << std::endl;
        }
        for (const auto &kv : smap) {
            std::cout << kv.first << "," << kv.second << std::endl;
        }
        smap.Destroy();
    }
}

// 文件dump uint64_t
TEST_F(SharedMapTest, test_static_Dump64) {
    std::string name = "./shashmap64.dat";
    bool res = levin::SharedHashMap<uint64_t, uint64_t>::Dump(name, map_kv64);
    EXPECT_TRUE(res);
}

// 不存在文件，返回文件不存在
TEST_F(SharedMapTest, test_Load_fail) {
    std::string name = "./no_exist.dat";
    {
        levin::SharedHashMap<uint64_t, uint64_t> mymap(name);
        EXPECT_EQ(mymap.Init(), SC_RET_FILE_NOEXIST);

        mymap.Destroy();
    }
}

//type_hash不一致，加载失败
TEST_F(SharedMapTest, test_type_hash) {
    std::string name = "./shashmap64.dat";
    levin::SharedHashMap<uint32_t, uint64_t> mymap(name);
    ASSERT_EQ(mymap.Init(), SC_RET_OK);
    ASSERT_EQ(mymap.Load(), SC_RET_LOAD_FAIL);
    mymap.Destroy();
}

// 接口功能
TEST_F(SharedMapTest, test_Load64) {
    std::string name = "./shashmap64.dat";
    {
        levin::SharedHashMap<uint64_t, uint64_t> mymap(name);
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

        for(auto bit = mymap.begin(); bit !=mymap.end(); ++bit){
            //std::cout << bit->first << " === " << bit->second << std::endl;
        }
        LEVIN_CDEBUG_LOG("%s", mymap.layout().c_str());
        mymap.Destroy();
    }
}

// 校验文件写入共享内存大小与load后共享内存大小一致性
TEST_F(SharedMapTest, test_check_map_memsize) {
    std::string name = "./shashmap64.dat";
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
        levin::SharedHashMap<uint64_t, uint64_t> mymap(name);
        ASSERT_EQ(mymap.Init(), SC_RET_OK);
        ASSERT_EQ(mymap.Load(), SC_RET_OK);
        size_t load_size = mymap.load_shm_size();
        LEVIN_CDEBUG_LOG("file mem_size:%lu, load mem_size:%lu", mem_size, load_size);
        EXPECT_EQ(mem_size + fixed_len, load_size);

        mymap.Destroy();
    }
}

TEST_F(SharedMapTest, test_smap_diff) {
    std::vector<std::pair<uint64_t, uint32_t> > vec_key_val;
    srand((unsigned)time(NULL));

    std::string name = "./shashmapdiff.dat";
    std::map<uint64_t, uint32_t> stdmap;
    vec_key_val.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        vec_key_val.emplace_back(std::make_pair(rand()%count, rand()%count));
        stdmap[vec_key_val[i].first] = vec_key_val[i].second;
    }
    bool res = levin::SharedHashMap<uint64_t, uint32_t>::Dump(name, stdmap);
    EXPECT_TRUE(res);

    //load snmp
    levin::SharedHashMap<uint64_t, uint32_t> mymap(name);
    ASSERT_EQ(mymap.Init(), SC_RET_OK);
    ASSERT_EQ(mymap.Load(), SC_RET_OK);

    //diff value
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(stdmap[vec_key_val[i].first], mymap[vec_key_val[i].first]);
    }
    LEVIN_CDEBUG_LOG("%s", mymap.layout().c_str());
    mymap.Destroy();
}

// dump struct
struct st_name {
    char name[21];
    st_name(){
        memset(name, 0, sizeof(name));
    }
    st_name(const char *s){
        strncpy(name, s, 20);
        name[20] = '\0';
        //std::cout << "st_name:" << name << std::endl;
    }
};
std::map<uint32_t, st_name> map_struct;
TEST_F(SharedMapTest, test_Dump_struct) {
    std::string name = "./shashmap_struct.dat";

    map_struct.insert(std::pair<uint32_t, st_name>(1,st_name("unordered_map")));
    map_struct.insert(std::pair<uint32_t, st_name>(3,st_name("map")));
    map_struct.insert(std::pair<uint32_t, st_name>(9,st_name("hash_map")));
    map_struct.insert(std::pair<uint32_t, st_name>(23,st_name("string")));

    bool res = levin::SharedHashMap<uint32_t, st_name>::Dump(name, map_struct);
    EXPECT_TRUE(res);
}

TEST_F(SharedMapTest, test_Load_struct) {
    std::string name = "./shashmap_struct.dat";
     
    levin::SharedHashMap<uint32_t, st_name> mymap(name);
    ASSERT_EQ(mymap.Init(), SC_RET_OK);
    ASSERT_EQ(mymap.Load(), SC_RET_OK);

    EXPECT_EQ(mymap.size(), 4);

    const st_name &ss = mymap[23];
    ASSERT_STREQ(ss.name, "string");
    LEVIN_CDEBUG_LOG("get name:%s", ss.name);
    LEVIN_CDEBUG_LOG("%s", mymap.layout().c_str());
    mymap.Destroy();

}

// 文件dump uint32_t
TEST_F(SharedMapTest, test_static_Dump32) {
    std::string name = "./shashmap32.dat";
    bool res = levin::SharedHashMap<uint32_t, uint32_t>::Dump(name, map_kv32);
    EXPECT_TRUE(res);
}

class SharedNestedHashMapTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
    const size_t fixed_len = SharedBase::MetaSize() + SharedBase::HeaderSize();
};

TEST_F(SharedNestedHashMapTest, test_type_traits_assert) {
    // error type, compile error is expected
//    std::map<uint32_t, std::vector<Cat> > nmap_cat;
//    levin::SharedNestedHashMap<int32_t, Cat>::Dump("", nmap_cat);
//    levin::SharedNestedHashMap<uint32_t, uint64_t>::Dump("", nmap_cat);
//    std::unordered_map<uint64_t, std::vector<Cat> > nhashmap_cat;
//    levin::SharedNestedHashMap<int, Cat>::Dump("", nhashmap_cat);
//    levin::SharedNestedHashMap<uint64_t, int64_t>::Dump("", nhashmap_cat);
}

TEST_F(SharedNestedHashMapTest, test_static_Dump_Load_empty) {
    std::string name = "./snhashmap_empty.dat";
    // dump
    {
        std::unordered_map<int, std::vector<int> > in;
        bool succ = levin::SharedNestedHashMap<int, int>::Dump(name, in);
        EXPECT_TRUE(succ);
    }
    // load
    {
        levin::SharedNestedHashMap<int, int> snmap(name);
        bool succ = (snmap.Init() == SC_RET_OK && (snmap.IsExist() || snmap.Load() == SC_RET_OK));
        ASSERT_TRUE(succ);
        LEVIN_CDEBUG_LOG("%s", snmap.layout().c_str());
        EXPECT_TRUE(snmap.empty());
        EXPECT_EQ(snmap.size(), 0);
        EXPECT_TRUE(snmap.find(1) == snmap.end());
        for (auto it = snmap.begin(); it != snmap.end(); ++it) {
            std::cout << it->first << "," << it->second->size() << std::endl;
        }
        for (const auto &kv : snmap) {
            std::cout << kv.first << "," << kv.second->size() << std::endl;
        }
        snmap.Destroy();
    }
}

TEST_F(SharedNestedHashMapTest, test_static_Dump) {
    std::string name = "./snhashmap.dat";

    bool res = levin::SharedNestedHashMap<uint32_t, uint32_t>::Dump(name, nmap_data);
    EXPECT_TRUE(res);
}

TEST_F(SharedNestedHashMapTest, test_Load) {
    std::string name = "./snhashmap.dat";
    {
        levin::SharedNestedHashMap<uint32_t, uint32_t> mymap(name);
        ASSERT_EQ(mymap.Init(), SC_RET_OK);
        ASSERT_EQ(mymap.Load(), SC_RET_OK);

        EXPECT_EQ(mymap.size(), nmap_data.size());

        EXPECT_EQ(mymap.count(count - 1), 1);
        EXPECT_EQ(mymap.count(count), 0);

        levin::SharedNestedHashMap<uint32_t, uint32_t>::iterator bit = mymap.begin();
        std::stringstream ss;
        for (;bit != mymap.end();++bit) {
            ss.str("");
            auto &vec = *(bit->second);
            for (size_t i=0; i < vec.size(); i++) {
                ss << vec[i] << "\t";
                //std::cout << vec[i] << "\t";
            }
            LEVIN_CDEBUG_LOG("%s",ss.str().c_str());
            //std::cout << std::endl;
        }
        LEVIN_CDEBUG_LOG("%s", mymap.layout().c_str());
        mymap.Destroy();
    }
    // SharedNestedHashMap in heap
    {
        auto map_ptr = new levin::SharedNestedHashMap<uint32_t, uint32_t>(name);
        ASSERT_EQ(map_ptr->Init(), SC_RET_OK);
        ASSERT_EQ(map_ptr->Load(), SC_RET_OK);
        map_ptr->Destroy();
    }
}

//校验文件写入共享内存大小与load后共享内存大小一致性
TEST_F(SharedNestedHashMapTest, test_check_nmap_memsize) {
    std::string name = "./snhashmap.dat";
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
        levin::SharedNestedHashMap<uint32_t, uint32_t> mymap(name);
        ASSERT_EQ(mymap.Init(), SC_RET_OK);
        ASSERT_EQ(mymap.Load(), SC_RET_OK);
        
        size_t load_size = mymap.load_shm_size();
        LEVIN_CDEBUG_LOG("file mem_size:%lu, load mem_size:%lu", mem_size, load_size);
        EXPECT_EQ(mem_size + fixed_len, load_size);

        mymap.Destroy();
    }
}

TEST_F(SharedNestedHashMapTest, test_snmap_diff) {
    std::vector<std::pair<uint64_t, uint32_t> > vec_key_val;
    srand((unsigned)time(NULL));

    std::string name = "./snhashmapdiff.dat";
    std::map<uint64_t, std::vector<uint32_t> > stdmap;
    vec_key_val.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        vec_key_val.emplace_back(std::make_pair(rand()%count, rand()%count));

        std::vector<uint32_t> vec(vec_key_val[i].second, i);
        stdmap[vec_key_val[i].first].assign(vec.begin(), vec.end());
    }
    bool res = levin::SharedNestedHashMap<uint64_t, uint32_t>::Dump(name, stdmap);
    EXPECT_TRUE(res);

    //load snmp
    levin::SharedNestedHashMap<uint64_t, uint32_t> mymap(name);
    ASSERT_EQ(mymap.Init(), SC_RET_OK);
    ASSERT_EQ(mymap.Load(), SC_RET_OK);

    //diff nested value
    for (size_t i = 0; i < count; ++i) {
        std::string std_str(stdmap[vec_key_val[i].first].begin(), stdmap[vec_key_val[i].first].end());
        std::string shm_str(mymap[vec_key_val[i].first]->begin(), mymap[vec_key_val[i].first]->end());
        EXPECT_STREQ(std_str.c_str(), shm_str.c_str());
    }
    mymap.Destroy();
}

}  // namespace levin

int main(int argc, char** argv) {
    //mock data
    levin::make_mapdata();

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
