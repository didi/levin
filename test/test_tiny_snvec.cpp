#include "svec.hpp"
#include <vector>
#include <gtest/gtest.h>
#include "test_header.h"

namespace levin {

class SharedTinyNestedVectorTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(SharedTinyNestedVectorTest, test_type_traits_assert) {
    // error SizeType, compile error is expected
    // levin::SharedNestedVector<int, int32_t> signedint_vec_vec("");
    // levin::SharedNestedVector<int, float> float_vec_vec("");
    // levin::SharedNestedVector<int, char> char_vec_vec("");
}

TEST_F(SharedTinyNestedVectorTest, test_static_Dump_Load_large_empty) {
    std::string name = "./large_nestvec_empty.dat";
    // dump
    {
        std::vector<std::vector<uint64_t> > in;
        for(auto &row : in) {
            for (auto & item : row) {
                std::cout << item << std::endl;
            }
        }
        bool succ = levin::SharedNestedVector<uint64_t, size_t>::Dump(name, in);
        EXPECT_TRUE(succ);
    }
    // load
    {
        levin::SharedNestedVector<uint64_t, size_t> vec(name);
        EXPECT_EQ(vec.Init(), SC_RET_OK);
        EXPECT_EQ(vec.Load(), SC_RET_OK);
        std::cout << "begin=" << vec.begin() << "\t" << vec.cbegin() << std::endl;
        std::cout << "end=" << vec.end() << "\t" << vec.cend() << std::endl;
        for(auto &row : vec) {
            for (auto & item : row) {
                std::cout << item << std::endl;
            }
        }
        LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());
        vec.Destroy();
    }
}

TEST_F(SharedTinyNestedVectorTest, test_static_Dump_Load_tiny_empty) {
    std::string name = "./tiny_nestvec_empty.dat";
    // dump
    {
        std::vector<std::vector<uint64_t> > in;
        for(auto &row : in) {
            for (auto & item : row) {
                std::cout << item << std::endl;
            }
        }
        bool succ = levin::SharedNestedVector<uint64_t>::Dump(name, in);
        EXPECT_TRUE(succ);
    }
    // load
    {
        levin::SharedNestedVector<uint64_t> vec(name);
        EXPECT_EQ(vec.Init(), SC_RET_OK);
        EXPECT_EQ(vec.Load(), SC_RET_OK);
        std::cout << "begin=" << vec.begin() << "\t" << vec.cbegin() << std::endl;
        std::cout << "end=" << vec.end() << "\t" << vec.cend() << std::endl;
        for(auto &row : vec) {
            for (auto & item : row) {
                std::cout << item << std::endl;
            }
        }
        LEVIN_CDEBUG_LOG("%s", vec.layout().c_str());
        vec.Destroy();
    }
}

TEST_F(SharedTinyNestedVectorTest, test_static_Dump_large) {
    std::string name = "./large_nestvec.dat";
    std::vector<std::vector<uint64_t> > in = {
        {1, 2, 3},
        {4, 5, 6, 7, 8, 9, 10},
        {100, 101, 102, 103},
        {1000, 1001}
    };

    bool succ = levin::SharedNestedVector<uint64_t, size_t>::Dump(name, in);
    EXPECT_TRUE(succ);
}

TEST_F(SharedTinyNestedVectorTest, test_Load_large) {
    // load succ
    {
        std::string name = "./large_nestvec.dat";
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
    // invalid SizeType, load fail
    {
        std::string name = "./large_nestvec.dat";
        levin::SharedNestedVector<uint64_t, uint16_t> vec(name);
        EXPECT_EQ(vec.Init(), SC_RET_OK);
        EXPECT_EQ(vec.Load(), SC_RET_LOAD_FAIL);
    }
}

TEST_F(SharedTinyNestedVectorTest, test_static_Dump_exceed_limit) {
    std::string name = "./exceed_nestvec.dat";
    std::vector<std::vector<uint64_t> > in(1, std::vector<uint64_t>(256, 77));

    // exceed numeric limit, Dump fail
    bool succ = levin::SharedNestedVector<uint64_t, uint8_t>::Dump(name, in);
    EXPECT_FALSE(succ);
}

TEST_F(SharedTinyNestedVectorTest, test_static_Dump_tiny) {
    std::string name = "./tiny_nestvec.dat";
    std::vector<std::vector<uint64_t> > in = {
        {1, 2, 3},
        {4, 5, 6, 7, 8, 9, 10},
        {100, 101, 102, 103},
        {1000, 1001}
    };

    bool succ = levin::SharedNestedVector<uint64_t>::Dump(name, in);
    EXPECT_TRUE(succ);
}

TEST_F(SharedTinyNestedVectorTest, test_Load_tiny) {
    // load succ
    {
        std::string name = "./tiny_nestvec.dat";
        levin::SharedNestedVector<uint64_t, uint32_t, SharedMemory, Md5Checker> vec(name);
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
    // invalid SizeType, load fail
    {
        std::string name = "./tiny_nestvec.dat";
        levin::SharedNestedVector<uint64_t, uint64_t> vec(name);
        EXPECT_EQ(vec.Init(), SC_RET_OK);
        EXPECT_EQ(vec.Load(), SC_RET_LOAD_FAIL);
    }
}

TEST_F(SharedTinyNestedVectorTest, test_tiny_diff) {
    std::string name = "./tiny_nestvec_diff.dat";
    std::vector<std::vector<uint64_t> > in = {
        {1, 2, 3},
        {4, 5, 6, 7, 8, 9, 10},
        {},
        {100, 101, 102, 103},
        {777},
        {1000, 1001}
    };

    bool succ = levin::SharedNestedVector<uint64_t, uint32_t>::Dump(name, in);
    EXPECT_TRUE(succ);
    levin::SharedNestedVector<uint64_t, uint32_t> vec(name);
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
