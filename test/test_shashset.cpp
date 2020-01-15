#include "shashset.hpp"
#include <unordered_set>
#include <gtest/gtest.h>
#include "test_header.h"

namespace levin {

class SharedHashSetTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(SharedHashSetTest, test_type_traits_assert) {
    // error type, compile error is expected
//    levin::SharedHashSet<std::set<int> > stdset_set("");
//    levin::SharedHashSet<std::string> stdstr_set("");
//    levin::SharedHashSet<std::map<int, int> > stdmap_set("");
//    levin::SharedHashSet<std::unordered_map<int, int> > stdhashmap_set("");
//    levin::SharedHashSet<SharedHashSet<int> > sset_set("");
//    levin::SharedHashSet<levin::HashSet<int> > chset_set("");
//    levin::SharedHashSet<levin::CustomVector<int> > cvec_set("");  -- passed but no Dump defined
}

TEST_F(SharedHashSetTest, test_static_Dump_Load_empty) {
    std::string name = "./hashset_empty.dat";
    // dump
    {
        std::unordered_set<int> in;
        // range-based traversal
        for(auto &item : in) {
            std::cout << item << std::endl;
        }
        // traversal by iterator
        for(auto it = in.begin(); it != in.end(); ++it) {
            std::cout << *it << std::endl;
        }
        EXPECT_TRUE(levin::SharedHashSet<int>::Dump(name, in));
    }
    // load
    {
        levin::SharedHashSet<
            int,
            std::hash<int>,
            std::equal_to<int>,
            levin::SharedMemory,
            levin::IntegrityChecker> hset(name);
        EXPECT_EQ(hset.Init(), SC_RET_OK);
        EXPECT_EQ(hset.Load(), SC_RET_OK);
        LEVIN_CDEBUG_LOG("%s", hset.layout().c_str());
        // range-based traversal
        for (auto &item : hset) {
            std::cout << item << std::endl;
        }
        // traversal by iterator
        for(auto it = hset.begin(); it != hset.end(); ++it) {
            std::cout << *it << std::endl;
        }
        hset.Destroy();
    }
}

TEST_F(SharedHashSetTest, test_static_Dump) {
    std::string name = "./hashset.dat";
    std::unordered_set<int> in;
    for (int i = 10; i >= 0; --i) {
        in.emplace(i);
    }
    in.emplace(-1);
    in.emplace(-100);
    in.emplace(-300);
    in.emplace(-777);
    // range-based traversal
    for (auto &item : in) {
        std::cout << item << " ";
    }
    std::cout << std::endl;
    // traversal by iterator
    for(auto it = in.begin(); it != in.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    std::cout << "find(9)==end is " << (in.find(9) == in.end())
              << " count(9)=" << in.count(9) << std::endl;
    std::cout << "find(-778)==end is " << (in.find(-778) == in.end())
              << " count(-778)=" << in.count(-778) << std::endl;
    EXPECT_TRUE(levin::SharedHashSet<int>::Dump(name, in));
}

TEST_F(SharedHashSetTest, test_Load) {
    std::string name = "./hashset.dat";
    // SharedHashSet in stack
    {
        levin::SharedHashSet<int/*, levin::IntegrityChecker*/> hset(name);
        bool succ = false;
        if (hset.Init() == SC_RET_OK && (hset.IsExist() || hset.Load() == SC_RET_OK)) {
            succ = true;
            LEVIN_CDEBUG_LOG("begin=%p, end=%p", hset.begin(), hset.end());
            // range-based traversal
            for (auto &item : hset) {
                std::cout << item << " ";
            }
            std::cout << std::endl;
            // traversal by iterator
            for(auto it = hset.begin(); it != hset.end(); ++it) {
                std::cout << *it << " ";
            }
            std::cout << std::endl;
            LEVIN_CDEBUG_LOG("%s", hset.layout().c_str());
        }
        EXPECT_TRUE(succ);
        hset.Destroy();
    }
    // SharedHashSet in heap
    {
        auto hset = new levin::SharedHashSet<
            int,
            std::hash<int>,
            std::equal_to<int>,
            levin::SharedMemory,
            levin::Md5Checker>(name);
        bool succ = false;
        if (hset->Init() == SC_RET_OK && (hset->IsExist() || hset->Load() == SC_RET_OK)) {
            succ = true;
            LEVIN_CDEBUG_LOG("cbegin=%p, cend=%p", hset->cbegin(), hset->cend());
            // range-based traversal
            for (auto &item : *hset) {
                std::cout << item << " ";
            }
            std::cout << std::endl;
            std::cout << "count(0)=" << hset->count(0) << std::endl;
            std::cout << "find(9)==end is " << (hset->find(9) == hset->end())
                      << " count(9)=" << hset->count(9) << std::endl;
            std::cout << "count(11)=" << hset->count(11) << std::endl;
            std::cout << "find(-1)==end is " << (hset->find(-1) == hset->end())
                      << " count(-1)=" << hset->count(-1) << std::endl;
            std::cout << "count(-100)=" << hset->count(-100) << std::endl;
            std::cout << "count(-300)=" << hset->count(-300) << std::endl;
            std::cout << "count(-777)=" << hset->count(-777) << std::endl;
            std::cout << "find(-778)==end is " << (hset->find(-778) == hset->end())
                      << " count(-778)=" << hset->count(-778) << std::endl;
            LEVIN_CDEBUG_LOG("%s", hset->layout().c_str());
        }
        EXPECT_TRUE(succ);
        hset->Destroy();
        delete hset;
    }
}

TEST_F(SharedHashSetTest, test_static_Dump_struct) {
    std::string name = "./cat_hashset.dat";
    std::vector<Cat> vec = { {"miaoji"}, {"fm"}, {"mimi"}, {"dahuang"} };
    std::unordered_set<Cat, CatHash, CatEqual> in(vec.begin(), vec.end());
    for (const auto &cat : in) {
        LEVIN_CDEBUG_LOG("%p %s", (void*)&cat, cat.name);
    }
    bool ret = levin::SharedHashSet<Cat, CatHash, CatEqual>::Dump(name, in);
    EXPECT_TRUE(ret);
}

TEST_F(SharedHashSetTest, test_Load_struct) {
    std::string name = "./cat_hashset.dat";
    {
        levin::SharedHashSet<Cat, CatHash, CatEqual, SharedMemory, Md5Checker> st(name);
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
        st.Destroy();
    }
}

TEST_F(SharedHashSetTest, test_diff) {
    std::string name = "./hashset_diff.dat";
    std::unordered_set<int> in;
    for (size_t i = 0; i < 100; ++i) {
        in.emplace(rand() % 100);
        in.emplace(0 - rand() % 100);
    }
    EXPECT_TRUE(levin::SharedHashSet<int>::Dump(name, in));

    levin::SharedHashSet<int> hset(name);
    ASSERT_TRUE(hset.Init() == SC_RET_OK && (hset.IsExist() || hset.Load() == SC_RET_OK));
    EXPECT_EQ(in.size(), hset.size());
    // DIFF find method
    for (auto &item : in) {
        std::cout << item << " ";
        auto it = hset.find(item);
        EXPECT_TRUE(it != hset.cend());
        EXPECT_EQ(*it, item);
    }
    std::cout << std::endl;
    for (size_t miss = 100; miss < 200; ++miss) {
        EXPECT_EQ((hset.find(miss) == hset.cend()), (in.find(miss) == in.cend()));
    }
    // DIFF count method
    for (auto &item : in) {
        EXPECT_EQ(hset.count(item), 1);
    }
    for (size_t miss = 100; miss < 200; ++miss) {
        EXPECT_EQ(hset.count(miss), 0);
    }
    // DIFF bucket size
    for (unsigned i = 0; i < in.bucket_count(); ++i) {
        if (in.bucket_size(i) > 1) {
            std::cout << "bucket #" << i << " has " << in.bucket_size(i) << " elements.\n";
        }
    }
    for (unsigned i = 0; i < hset.bucket_count(); ++i) {
        if (hset.bucket_size(i) > 1) {
            std::cout << "bucket #" << i << " has " << hset.bucket_size(i) << " elements.\n";
        }
    }
    //std::cout << "bucket #" << in.bucket_count() << " has "
    //          << in.bucket_size(in.bucket_count()) << " elements.\n";
    //std::cout << "bucket #" << hset.bucket_count() << " has "
    //          << hset.bucket_size(hset.bucket_count()) << " elements.\n";

    hset.Destroy();
}

class HashSetTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(HashSetTest, test_default_construct) {
    // default construct
    {
        void *addr = malloc(1024);
        HashSet<int> *set_ptr = new(addr) HashSet<int>;
        EXPECT_EQ(set_ptr->bucket_count(), 1);
        EXPECT_EQ(set_ptr->size(), 0);
        EXPECT_TRUE(set_ptr->empty());
        EXPECT_EQ(set_ptr->datas().size(), 1);
        EXPECT_EQ(set_ptr->begin(), set_ptr->end());
        EXPECT_EQ(set_ptr->cbegin(), set_ptr->cend());
        std::cout << "begin=" << set_ptr->begin() << "\t" << set_ptr->cbegin() << std::endl;
        std::cout << "end=" << set_ptr->end() << "\t" << set_ptr->cend() << std::endl;
        // range-based traversal
        for (auto &item : *set_ptr) {
            std::cout << item << std::endl;
        }
        // traversal by iterator
        for(auto it = set_ptr->begin(); it != set_ptr->end(); ++it) {
            std::cout << *it << std::endl;
        }
        LEVIN_CDEBUG_LOG("%s", set_ptr->layout().c_str());

        set_ptr->~HashSet<int>();
        free(addr);
    }
}

}  // namespace levin

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
