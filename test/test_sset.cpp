#include "sset.hpp"
#include <set>
#include <unordered_map>
#include <gtest/gtest.h>
#include "test_header.h"

namespace levin {

class SharedSetTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(SharedSetTest, test_type_traits_assert) {
    // error type, compile error is expected
//    levin::SharedSet<std::set<int> > stdset_set("");
//    levin::SharedSet<std::string> stdstr_set("");
//    levin::SharedSet<std::map<int, int> > stdmap_set("");
//    levin::SharedSet<std::unordered_map<int, int> > stdhashmap_set("");
//    levin::SharedSet<levin::SharedSet<int> > sset_set("");
//    levin::SharedSet<levin::HashSet<int> > shset_set("");
//    levin::SharedSet<levin::CustomVector<int> > cvec_set("");  -- passed but no Dump defined
}

TEST_F(SharedSetTest, test_static_Dump_Load_empty) {
    std::string name = "./set_empty.dat";
    // dump
    {
        std::set<int> in;
        EXPECT_TRUE(levin::SharedSet<int>::Dump(name, in));
    }
    // load
    {
        levin::SharedSet<int, std::less<int>, SharedMemory, IntegrityChecker> st(name);
        EXPECT_EQ(st.Init(), SC_RET_OK);
        EXPECT_EQ(st.Load(), SC_RET_OK);
        // range-based traversal
        for(auto &item : st) {
            std::cout << item << std::endl;
        }
        // traversal by iterator
        for(auto it = st.begin(); it != st.end(); ++it) {
            std::cout << *it << std::endl;
        }
        LEVIN_CDEBUG_LOG("%s", st.layout().c_str());
        st.Destroy();
    }
}

TEST_F(SharedSetTest, test_static_Dump) {
    std::string name = "./set.dat";
    std::set<int> in;
    for (int i = 10; i >= 0; --i) {
        in.emplace(i);
    }
    // range-based traversal
    for(auto &item : in) {
        std::cout << item << " ";
    }
    std::cout << std::endl;
    // traversal by iterator
    for(auto it = in.begin(); it != in.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    bool ret = levin::SharedSet<int>::Dump(name, in);
    EXPECT_TRUE(ret);
}

TEST_F(SharedSetTest, test_Load) {
    std::string name = "./set.dat";
    // SharedSet in stack
    {
        levin::SharedSet<int> st(name);
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
        st.Destroy();
    }
    // SharedSet in heap
    {
        auto st_ptr = new levin::SharedSet<int, std::less<int>, SharedMemory, Md5Checker>(name);
        EXPECT_EQ(st_ptr->Init(), SC_RET_OK);
        EXPECT_EQ(st_ptr->Load(), SC_RET_OK);
        LEVIN_CDEBUG_LOG("%s", st_ptr->layout().c_str());
        st_ptr->Destroy();
        delete st_ptr;
    }
}

TEST_F(SharedSetTest, test_static_Dump_struct) {
    std::string name = "./cat.dat";
    std::vector<Cat> vec = { {"miaoji"}, {"fm"}, {"mimi"}, {"dahuang"} };
    std::set<Cat, CatComp> in(vec.begin(), vec.end());
    // range-based traversal
    for (const auto &cat : in) {
        std::cout << cat.name << " ";
    }
    std::cout << std::endl;
    bool ret = levin::SharedSet<Cat, CatComp>::Dump(name, in);
    EXPECT_TRUE(ret);
}

TEST_F(SharedSetTest, test_Load_struct) {
    std::string name = "./cat.dat";
    {
        levin::SharedSet<Cat, CatComp/*, levin::IntegrityChecker*/> st(name);
        bool succ = false;
        if (st.Init() == SC_RET_OK && (st.IsExist() || st.Load() == SC_RET_OK)) {
            succ = true;
            // range-based traversal
            for (const auto &cat : st) {
                std::cout << cat.name << " ";
            }
            std::cout << std::endl;
            // traversal by iterator
            for(auto it = st.begin(); it != st.end(); ++it) {
                std::cout << it->name << " ";
            }
            std::cout << std::endl;
            LEVIN_CDEBUG_LOG("%s", st.layout().c_str());
        }
        EXPECT_TRUE(succ);
        st.Destroy();
    }
}

void diff_comparison(const std::set<int> &in, const levin::SharedSet<int> &st) {
    // DIFF lower_bound method max of <= val
    for (auto &item : in) {
        EXPECT_TRUE(st.lower_bound(item) != st.end());
        EXPECT_EQ(*st.lower_bound(item), *in.lower_bound(item));
    }
    for (size_t miss = 100; miss < 200; ++miss) {
        EXPECT_TRUE(st.lower_bound(miss) == st.end());
        EXPECT_TRUE(in.lower_bound(miss) == in.end());
    }
    // DIFF upper_bound method min of > val
    for (auto &item : in) {
        if (item == 99) {
            continue;
        }
        EXPECT_TRUE(st.upper_bound(item) != st.end());
        EXPECT_EQ(*st.upper_bound(item), *in.upper_bound(item));
    }
    for (size_t miss = 99; miss < 200; ++miss) {
        EXPECT_TRUE(st.upper_bound(miss) == st.end());
        EXPECT_TRUE(in.upper_bound(miss) == in.end());
    }
    // DIFF equal_range method [,)
    for (auto &item : in) {
        if (item == 99) {
            continue;
        }
        auto pair = st.equal_range(item);
        EXPECT_TRUE(pair.first != st.end());
        EXPECT_TRUE(pair.second != st.end());
        auto range = in.equal_range(item);
        EXPECT_TRUE(range.first != in.end());
        EXPECT_TRUE(range.second != in.end());
        EXPECT_EQ(*pair.first, *range.first);
        EXPECT_EQ(*pair.second, *range.second);
    }
    {
        auto pair = st.equal_range(99);
        EXPECT_TRUE(pair.first != st.end());
        EXPECT_TRUE(pair.second == st.end());
        auto range = in.equal_range(99);
        EXPECT_TRUE(range.first != in.end());
        EXPECT_TRUE(range.second == in.end());
        EXPECT_EQ(*pair.first, *range.first);
    }
    for (size_t miss = 100; miss < 200; ++miss) {
        auto pair = st.equal_range(miss);
        EXPECT_TRUE(pair.first == st.end());
        EXPECT_TRUE(pair.second == st.end());
        auto range = in.equal_range(miss);
        EXPECT_TRUE(range.first == in.end());
        EXPECT_TRUE(range.second == in.end());
    }
}

TEST_F(SharedSetTest, test_diff) {
    std::string name = "./set_diff.dat";
    std::set<int> in;
    for (size_t i = 0; i < 100; ++i) {
        in.emplace(rand() % 100);
    }
    EXPECT_TRUE(levin::SharedSet<int>::Dump(name, in));

    levin::SharedSet<int> st(name);
    ASSERT_TRUE(st.Init() == SC_RET_OK && (st.IsExist() || st.Load() == SC_RET_OK));
    EXPECT_EQ(in.size(), st.size());
    // DIFF find method
    for (auto &item : in) {
        std::cout << item << " ";
        auto it = st.find(item);
        EXPECT_TRUE(it != st.end());
        EXPECT_EQ(*it, item);
        EXPECT_TRUE(in.find(item) != in.end());
    }
    std::cout << std::endl;
    for (size_t miss = 100; miss < 200; ++miss) {
        EXPECT_EQ((st.find(miss) == st.cend()), (in.find(miss) == in.cend()));
    }
    // DIFF count method
    for (auto &item : in) {
        EXPECT_EQ(st.count(item), 1);
    }
    for (size_t miss = 100; miss < 200; ++miss) {
        EXPECT_EQ(st.count(miss), 0);
    }
    // DIFF access method iterator
    auto exp_it = in.cbegin();
    auto it = st.cbegin();
    for (; exp_it != in.cend() && it != st.cend(); ++exp_it, ++it) {
        EXPECT_EQ(*it, *exp_it);
    }
    diff_comparison(in, st);

    st.Destroy();
}

class CustomSetTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(CustomSetTest, test_default_construct) {
    // default construct
    {
        void *addr = malloc(1024);
        CustomSet<int> *set_ptr = new(addr) CustomSet<int>;
        EXPECT_EQ(set_ptr->size(), 0);
        EXPECT_TRUE(set_ptr->empty());
        EXPECT_EQ(set_ptr->data().size(), 0);
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

        set_ptr->~CustomSet<int>();
        free(addr);
    }
}

}  // namespace levin

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
