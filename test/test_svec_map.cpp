#include "svec_map.hpp"
#include <unordered_set>
#include <gtest/gtest.h>
#include "test_header.h"

namespace levin {

class SharedNestedMapTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(SharedNestedMapTest, test_type_traits_assert) {
    // error type, compile error is expected
//    levin::SharedNestedMap<std::string, int> strkey_vecmap("");
//    levin::SharedNestedMap<int, std::string> strval_vecmap("");
//    levin::SharedNestedMap<int, levin::SharedVector<int> > svec_vecmap("");
//    levin::SharedNestedMap<int, levin::CustomVector<int> > cvec_vecmap(""); -- passed but no Dump defined
}

TEST_F(SharedNestedMapTest, test_static_Dump_Load_empty) {
    std::string name = "./vecmap_empty.dat";
    // dump
    {
        std::vector<std::map<double, double> > in;
        // range-based traversal
        for (auto &map : in) {
            for (auto &pair : map) {
                std::cout << pair.first << "," << pair.second << std::endl;
            }
        }
        // traversal by iterator
        for (auto it = in.begin(); it != in.end(); ++it) {
            for (auto pit = it->begin(); pit != it->end(); ++pit) {
                std::cout << pit->first << "," << pit->second << std::endl;
            }
        }
        bool succ = levin::SharedNestedMap<double, double>::Dump(name, in);
        EXPECT_TRUE(succ);
    }
    // load
    {
        levin::SharedNestedMap<double, double> vecmap(name);
        EXPECT_EQ(vecmap.Init(), SC_RET_OK);
        EXPECT_EQ(vecmap.Load(), SC_RET_OK);
        LEVIN_CDEBUG_LOG("%s", vecmap.layout().c_str());
        // range-based traversal
        for (auto &mp : vecmap) {
            for (auto &pair : mp) {
                std::cout << pair.first << "," << pair.second << std::endl;
            }
        }
        // traversal by iterator
        for (auto it = vecmap.cbegin(); it != vecmap.cend(); ++it) {
            for (auto pit = it->cbegin(); pit != it->cend(); ++pit) {
                std::cout << pit->first << "," << pit->second << std::endl;
            }
        }
        vecmap.Destroy();
    }
}

TEST_F(SharedNestedMapTest, test_static_Dump) {
    std::string name = "./vecmap.dat";
    std::vector<std::map<double, double> > in = {
        { {0.3, 1.5}, {0.1, 1}, {0.5, 2} },
        {},
        { {0.9, 4.5}, {0.7, 3.5}, {0.8, 4.0}, {0.6, 3.0} },
        { {1.0, 5.0} },
        {}
    };
    // range-based traversal
    for (auto &map : in) {
        for (auto &pair : map) {
            std::cout << pair.first << "," << pair.second << std::endl;
        }
    }
    std::cout << std::endl;
    // traversal by iterator
    for (auto it = in.cbegin(); it != in.cend(); ++it) {
        for (auto pit = it->cbegin(); pit != it->cend(); ++pit) {
            std::cout << pit->first << "," << pit->second << std::endl;
        }
    }
    std::cout << std::endl;
    bool succ = levin::SharedNestedMap<double, double>::Dump(name, in);
    EXPECT_TRUE(succ);
}

TEST_F(SharedNestedMapTest, test_Load) {
    std::string name = "./vecmap.dat";
    // SharedNestedMap in stack
    {
        levin::SharedNestedMap<double, double> vecmap(name);
        bool succ = false;
        if (vecmap.Init() == SC_RET_OK && (vecmap.IsExist() || vecmap.Load() == SC_RET_OK)) {
            succ = true;
            LEVIN_CDEBUG_LOG("begin=%p, end=%p", vecmap.begin(), vecmap.end());
            // range-based traversal
            for (const auto &mp : vecmap) {
                std::cout << "map size=" << mp.size() << std::endl;
                for (const auto &pair : mp) {
                    std::cout << pair.first << "," << pair.second << std::endl;
                }
            }
            std::cout << std::endl;
            // traversal by iterator
            for (auto it = vecmap.cbegin(); it != vecmap.cend(); ++it) {
                for (auto pit = it->cbegin(); pit != it->cend(); ++pit) {
                    std::cout << pit->first << "," << pit->second << std::endl;
                }
            }
            std::cout << std::endl;
            LEVIN_CDEBUG_LOG("%s", vecmap.layout().c_str());
        }
        EXPECT_TRUE(succ);
        vecmap.Destroy();
    }
}

TEST_F(SharedNestedMapTest, test_diff) {
    std::string name = "./vecmap_diff.dat";
    std::vector<std::map<uint32_t, double> > in(10);
    for (size_t row = 0; row < in.size(); ++row) {
        auto &map = in[row];
        for (size_t i = 0; i < 100; ++i) {
            uint32_t key = rand() % 100;
            map.emplace(std::make_pair(key, double(key) / 100.0f));
        }
    }
    bool succ = levin::SharedNestedMap<uint32_t, double>::Dump(name, in);
    EXPECT_TRUE(succ);

    levin::SharedNestedMap<uint32_t, double> vecmap(name);
    succ = (vecmap.Init() == SC_RET_OK && (vecmap.IsExist() || vecmap.Load() == SC_RET_OK));
    ASSERT_TRUE(succ);
    LEVIN_CDEBUG_LOG("%s", vecmap.layout().c_str());
    EXPECT_EQ(in.size(), vecmap.size());

    // DIFF find&count method
    // hit case
    for (size_t idx = 0; idx < in.size(); ++idx) {
        const auto &stdmap = in[idx];
        const auto &smap = vecmap[idx];
        for (auto &pair : stdmap) {
            uint32_t key = pair.first;
            auto it = smap.find(key);
            EXPECT_TRUE(it != smap.cend());
            EXPECT_EQ(it->second, pair.second);
            EXPECT_EQ(stdmap.count(key), smap.count(key));
        }
    }
    std::cout << std::endl;
    // miss case
    for (size_t idx = 0; idx < in.size(); ++idx) {
        const auto &stdmap = in[idx];
        const auto &smap = vecmap[idx];
        for (size_t miss = 100; miss < 200; ++miss) {
            EXPECT_EQ((smap.find(miss) == smap.cend()), (stdmap.find(miss) == stdmap.cend()));
            EXPECT_EQ(stdmap.count(miss), 0);
            EXPECT_EQ(smap.count(miss), 0);
        }
    }

    // DIFF bucket lower_bound&upper_bound
    // TODO
    vecmap.Destroy();
}

}  // namespace levin

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
