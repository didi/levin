#include "sset.hpp"
#include "smap.hpp"
#include "shashset.hpp"
#include "shashmap.hpp"
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <gtest/gtest.h>
#include "test_header.h"

namespace levin {

class HeapMemoryTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(HeapMemoryTest, test_sset) {
    std::string name = "./heap_set.dat";
    {
        std::set<int> in = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
        EXPECT_TRUE(levin::SharedSet<int>::Dump(name, in));
    }
    // SharedSet alloc in heap memory
    {
        levin::SharedSet<int, std::less<int>, HeapMemory, Md5Checker> st(name);
        EXPECT_EQ(st.Init(), SC_RET_OK);
        EXPECT_EQ(st.Load(), SC_RET_OK);
        // range-based traversal
        for(auto &item : st) {
            std::cout << item << " ";
        }
        std::cout << std::endl;
        // traversal by iterator
        for(auto it = st.begin(); it != st.end(); ++it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
        LEVIN_CDEBUG_LOG("%s", st.layout().c_str());
//        st.Destroy();
    }
}

TEST_F(HeapMemoryTest, test_smap) {
    srand((unsigned)time(NULL));

    std::string name = "./heap_smap.dat";
    std::map<uint64_t, uint32_t> in = {
        {1111, 1}, {2222, 2}, {3333, 3}, {4444, 4}, {5555, 5}
    };
    bool ret = levin::SharedMap<uint64_t, uint32_t>::Dump(name, in);
    EXPECT_TRUE(ret);

    //load smap
    levin::SharedMap<uint64_t, uint32_t, std::less<uint64_t>, HeapMemory> mymap(name);
    ASSERT_EQ(mymap.Init(), SC_RET_OK);
    ASSERT_EQ(mymap.Load(), SC_RET_OK);

    //diff value
    for (const auto &kv : in) {
        EXPECT_EQ(kv.second, mymap[kv.first]);
    }
//    mymap.Destroy();
}

TEST_F(HeapMemoryTest, test_heap_shashset) {
    std::string name = "./heap_cat_hashset.dat";
    {
        std::vector<Cat> vec = { {"miaoji"}, {"fm"}, {"mimi"}, {"dahuang"} };
        std::unordered_set<Cat, CatHash, CatEqual> in(vec.begin(), vec.end());
        bool ret = levin::SharedHashSet<Cat, CatHash, CatEqual>::Dump(name, in);
        EXPECT_TRUE(ret);
    }
    {
        levin::SharedHashSet<Cat, CatHash, CatEqual, HeapMemory, Md5Checker> st(name);
        bool succ = false;
        if (st.Init() == SC_RET_OK && (st.IsExist() || st.Load() == SC_RET_OK)) {
            succ = true;
            std::cout << "count(miaoji)=" << st.count(Cat("miaoji")) << std::endl;
            std::cout << "count(fm)=" << st.count(Cat("fm")) << std::endl;
            std::cout << "count(mimi)=" << st.count(Cat("mimi")) << std::endl;
            std::cout << "count(dahuang)=" << st.count(Cat("dahuang")) << std::endl;
            // traverse elements
            for (const auto &cat : st) {
                LEVIN_CDEBUG_LOG("%p %s", (void*)&cat, cat.name);
            }
            LEVIN_CDEBUG_LOG("%s", st.layout().c_str());
        }
        EXPECT_TRUE(succ);
//        st.Destroy();
    }
}

TEST_F(HeapMemoryTest, test_shashmap) {
    std::string name = "./heap_shashmap64.dat";
    std::map<uint64_t, uint64_t> in = {
        {11, 77}, {77, 321}, {111, 777}, {1024, 2048}, {10000, 11111}, {77777, 88888}
    };
    bool ret = levin::SharedHashMap<uint64_t, uint64_t>::Dump(name, in);
    EXPECT_TRUE(ret);
    // alloc memory in heap
    {
        levin::SharedHashMap<uint64_t, uint64_t, std::hash<uint64_t>, HeapMemory, IntegrityChecker> mymap(name);
        ASSERT_EQ(mymap.Init(), SC_RET_OK);
        ASSERT_EQ(mymap.Load(), SC_RET_OK);

        EXPECT_EQ(mymap.size(), in.size());
        EXPECT_FALSE(mymap.empty());

        for (const auto &kv : in) {
            ASSERT_EQ(1, mymap.count(kv.first));
        }

        for(auto it = mymap.begin(); it !=mymap.end(); ++it){
            std::cout << it->first << " === " << it->second << std::endl;
        }
        LEVIN_CDEBUG_LOG("%s", mymap.layout().c_str());
//        mymap.Destroy();
    }
}

TEST_F(HeapMemoryTest, test_Load_nestvec) {
    std::string name = "./heap_nestvec.dat";
    {
        std::vector<std::vector<uint64_t> > in = {
            {1, 2, 3},
            {4, 5, 6, 7, 8, 9, 10},
            {100, 101, 102, 103},
            {1000, 1001}
        };
        bool succ = levin::SharedNestedVector<uint64_t, size_t>::Dump(name, in);
        EXPECT_TRUE(succ);
    }
    // load at heap
    {
        levin::SharedNestedVector<uint64_t, size_t, HeapMemory> vec(name);
        bool succ = false;
        if (vec.Init() == SC_RET_OK && (vec.IsExist() || vec.Load() == SC_RET_OK)) {
            succ = true;
            LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());
            // traverse elements
            for (auto row = vec.cbegin(); row != vec.cend(); ++row) {
                for (auto it = row->cbegin(); it != row->cend(); ++it) {
                    std::cout << *it << "\t";
                }
                std::cout << std::endl;
            }
        }
        EXPECT_TRUE(succ);
//        vec.Destroy();
    }
}

}  // namespace levin

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
