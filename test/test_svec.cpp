#include "svec.hpp"
#include <vector>
#include <gtest/gtest.h>
#include "test_header.h"

namespace levin {

class SharedVectorTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(SharedVectorTest, test_type_traits_assert) {
    // error type, compile error is expected
//    levin::SharedVector<std::vector<int> > stdvec_vec("");
//    levin::SharedVector<std::string> stdstr_vec("");
//    levin::SharedVector<std::map<int, int> > stdmap_vec("");
//    levin::SharedVector<std::unordered_map<int, int> > stdhashmap_vec("");
//    levin::SharedVector<SharedVector<int> > svec_vec("");
//    levin::SharedNestedVector<std::string> stdstr_vec_vec("");
//    levin::SharedNestedVector<std::vector<int> > stdvec_vec_vec("");
//    levin::SharedNestedVector<std::map<int, int> > stdmap_vec_vec("");
//    levin::SharedNestedVector<SharedVector<int> > cvec_vec_vec("");
}

TEST_F(SharedVectorTest, test_static_Dump_Load_empty) {
    std::string name = "./vec_empty.dat";
    // dump
    {
        std::vector<int> in;
        EXPECT_TRUE(levin::SharedVector<int>::Dump(name, in));
    }
    // load at heap
    {
        levin::SharedVector<int, HeapMemory> vec(name);
        EXPECT_EQ(vec.Init(), SC_RET_OK);
        EXPECT_EQ(vec.Load(), SC_RET_OK);
        LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());
        vec.Destroy();
    }
    // load
    {
        levin::SharedVector<int/*, levin::IntegrityChecker*/> vec(name);
        EXPECT_EQ(vec.Init(), SC_RET_OK);
        EXPECT_EQ(vec.Load(), SC_RET_OK);
        LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());
        vec.Destroy();
    }
}

TEST_F(SharedVectorTest, test_static_Dump) {
    std::string name = "./vec.dat";
    std::vector<int> in;
    for (int i = 10; i >= 0; --i) {
        in.emplace_back(i);
    }
    EXPECT_TRUE(levin::SharedVector<int>::Dump(name, in));
}

TEST_F(SharedVectorTest, test_Load) {
    std::string name = "./vec.dat";
    // SharedVector in stack
    {
        levin::SharedVector<int/*, levin::IntegrityChecker*/> vec(name);
        EXPECT_EQ(vec.Init(), SC_RET_OK);
        EXPECT_EQ(vec.Load(), SC_RET_OK);
        LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());
        vec.Destroy();
    }
    // SharedVector in heap
    {
        auto vec_ptr = new levin::SharedVector<int, SharedMemory, Md5Checker>(name);
        EXPECT_EQ(vec_ptr->Init(), SC_RET_OK);
        EXPECT_EQ(vec_ptr->Load(), SC_RET_OK);
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());
        vec_ptr->Destroy();
        delete vec_ptr;
    }
}

TEST_F(SharedVectorTest, test_static_Dump_struct) {
    std::string name = "./cat.dat";
    std::vector<Cat> in = { {"miaoji"}, {"fm"}, {"mimi"}, {"dahuang"} };
    EXPECT_TRUE(levin::SharedVector<Cat>::Dump(name, in));
}

TEST_F(SharedVectorTest, test_Load_struct) {
    std::string name = "./cat.dat";
    {
        levin::SharedVector<Cat/*, levin::IntegrityChecker*/> vec(name);
        bool succ = false;
        if (vec.Init() == SC_RET_OK && (vec.IsExist() || vec.Load() == SC_RET_OK)) {
            succ = true;
            LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());
            // traverse elements
            for (size_t i = 0; i < vec.size(); ++i) {
                const Cat &cat = vec[i];
                LEVIN_CDEBUG_LOG("%p %s", (void*)&cat, cat.name);
            }
        }
        EXPECT_TRUE(succ);
        vec.Destroy();
    }
}

TEST_F(SharedVectorTest, test_diff) {
    std::string name = "./vec_diff.dat";
    std::vector<int> in;
    for (size_t i = 0; i < 100; ++i) {
        in.emplace_back(rand() % 100);
    }
    EXPECT_TRUE(levin::SharedVector<int>::Dump(name, in));

    levin::SharedVector<int> vec(name);
    ASSERT_TRUE(vec.Init() == SC_RET_OK && (vec.IsExist() || vec.Load() == SC_RET_OK));
    // DIFF access method operator[]
    for (size_t idx = 0; idx < vec.size(); ++idx) {
        EXPECT_EQ(in[idx], vec[idx]);
    }
    // DIFF access method iterator
    auto exp_it = in.cbegin();
    auto it = vec.cbegin();
    for (; exp_it != in.cend() && it != vec.cend(); ++exp_it, ++it) {
        EXPECT_EQ(*it, *exp_it);
    }
    vec.Destroy();
}

// ************** nested vector *********************** //
class SharedNestedVectorTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(SharedNestedVectorTest, test_static_Dump_Load_empty) {
    std::string name = "./nestvec_empty.dat";
    // dump
    {
        std::vector<std::vector<uint64_t> > in;
        EXPECT_TRUE(levin::SharedNestedVector<uint64_t>::Dump(name, in));
    }
    // load
    {
        levin::SharedNestedVector<uint64_t/*, levin::IntegrityChecker*/> vec(name);
        EXPECT_EQ(vec.Init(), SC_RET_OK);
        EXPECT_EQ(vec.Load(), SC_RET_OK);
        LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());
        vec.Destroy();
    }
}

TEST_F(SharedNestedVectorTest, test_static_Dump) {
    std::string name = "./nestvec.dat";
    std::vector<std::vector<uint64_t> > in = {
        {1, 2, 3},
        {4, 5, 6, 7, 8, 9, 10},
        {100, 101, 102, 103},
        {1000, 1001}
    };
    bool succ = levin::SharedNestedVector<uint64_t, size_t>::Dump(name, in);
    EXPECT_TRUE(succ);
}

TEST_F(SharedNestedVectorTest, test_Load) {
    // load succ
    {
        std::string name = "./nestvec.dat";
        levin::SharedNestedVector<uint64_t, size_t, SharedMemory, Md5Checker> vec(name);
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
        vec.Destroy();
    }
    // file noexist, load fail
    {
        std::string name = "./nestvec_noexist.dat";
        levin::SharedNestedVector<uint64_t/*, levin::IntegrityChecker*/> vec(name);
        EXPECT_EQ(vec.Init(), SC_RET_FILE_NOEXIST);
    }
}

TEST_F(SharedNestedVectorTest, test_diff) {
    std::string name = "./nestvec_diff.dat";
    std::vector<std::vector<uint64_t> > in = {
        {1, 2, 3},
        {4, 5, 6, 7, 8, 9, 10},
        {},
        {100, 101, 102, 103},
        {777},
        {1000, 1001}
    };

    EXPECT_TRUE(levin::SharedNestedVector<uint64_t>::Dump(name, in));
    levin::SharedNestedVector<uint64_t> vec(name);
    ASSERT_TRUE(vec.Init() == SC_RET_OK && (vec.IsExist() || vec.Load() == SC_RET_OK));
    // DIFF access method operator[]
    for (size_t row_idx = 0; row_idx < vec.size(); ++row_idx) {
        const auto &in_row = in[row_idx];
        const auto &vec_row = vec[row_idx];
        for (size_t col_idx = 0; col_idx < in_row.size(); ++col_idx) {
            std::cout << vec_row[col_idx] << "\t";
            EXPECT_EQ(in_row[col_idx], vec_row[col_idx]);
        }
        std::cout << std::endl;
    }
    // DIFF access method iterator
    auto exp_row = in.cbegin();
    auto row = vec.cbegin();
    for (; exp_row != in.cend() && row != vec.cend(); ++exp_row, ++row) {
        auto exp_it = exp_row->cbegin();
        auto it = row->cbegin();
        for (; exp_it != exp_row->cend() && it != row->cend(); ++exp_it, ++it) {
            EXPECT_EQ(*it, *exp_it);
        }
    }
    vec.Destroy();
}

}  // namespace levin

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
