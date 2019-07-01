#include "details/vector.hpp"
#include <vector>
#include <gtest/gtest.h>
#include "svec.hpp"
#include "test_header.h"

namespace levin {

class CustomVectorTest : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(CustomVectorTest, test_default_construct) {
    // default construct
    {
        void *addr = malloc(sizeof(CustomVector<int>));
        CustomVector<int> *vec_ptr = new(addr) CustomVector<int>;
        EXPECT_EQ(vec_ptr->capacity(), 0);
        EXPECT_EQ(vec_ptr->size(), 0);
        EXPECT_EQ(vec_ptr->_arr_offset, sizeof(CustomVector<uint64_t>));
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());

        vec_ptr->~CustomVector<int>();
        free(addr);
    }
}

TEST_F(CustomVectorTest, test_construct_n) {
    // CustomVector(size_type n)
    {
        size_t N = 10;
        void *addr = malloc(sizeof(CustomVector<uint64_t>) + sizeof(uint64_t) * N);
        CustomVector<uint64_t> *vec_ptr = new(addr) CustomVector<uint64_t>(N);
        EXPECT_EQ(vec_ptr->capacity(), 10);
        EXPECT_EQ(vec_ptr->size(), 10);
        EXPECT_EQ(vec_ptr->_arr_offset, sizeof(CustomVector<uint64_t>));
        //EXPECT_EQ(vec_ptr->front(), 0);
        //EXPECT_EQ(vec_ptr->back(), 0);
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());

        vec_ptr->~CustomVector<uint64_t>();
        free(addr);
    }
    // CustomVector(size_type n) n = 0
    {
        size_t N = 0;
        void *addr = malloc(sizeof(CustomVector<uint64_t>) + sizeof(uint64_t) * N);
        CustomVector<uint64_t> *vec_ptr = new(addr) CustomVector<uint64_t>(N);
        EXPECT_EQ(vec_ptr->capacity(), 0);
        EXPECT_EQ(vec_ptr->size(), 0);
        EXPECT_EQ(vec_ptr->_arr_offset, sizeof(CustomVector<uint64_t>));
        EXPECT_TRUE(vec_ptr->empty());
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());

        vec_ptr->~CustomVector<uint64_t>();
        free(addr);
    }
}

TEST_F(CustomVectorTest, test_construct_lst) {
    // CustomVector(std::initializer_list<T> lst)
    {
        size_t N = 5;
        void *addr = malloc(sizeof(CustomVector<uint64_t>) + sizeof(uint64_t) * N);
        CustomVector<uint64_t> *vec_ptr = new(addr) CustomVector<uint64_t>({1, 2, 3, 4, 5});
        EXPECT_EQ(vec_ptr->capacity(), N);
        EXPECT_EQ(vec_ptr->size(), N);
        EXPECT_EQ(vec_ptr->_arr_offset, sizeof(CustomVector<uint64_t>));
        EXPECT_EQ(vec_ptr->front(), 1);
        EXPECT_EQ(vec_ptr->back(), 5);
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());

        vec_ptr->~CustomVector<uint64_t>();
        free(addr);
    }
    // CustomVector(std::initializer_list<T> lst) lst = {}
    {
        size_t N = 0;
        void *addr = malloc(sizeof(CustomVector<uint64_t>) + sizeof(uint64_t) * N);
        CustomVector<uint64_t> *vec_ptr = new(addr) CustomVector<uint64_t>({});
        EXPECT_EQ(vec_ptr->capacity(), N);
        EXPECT_EQ(vec_ptr->size(), N);
        EXPECT_EQ(vec_ptr->_arr_offset, sizeof(CustomVector<uint64_t>));
        EXPECT_TRUE(vec_ptr->empty());
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());

        vec_ptr->~CustomVector<uint64_t>();
        free(addr);
    }
}

TEST_F(CustomVectorTest, test_construct_n_val) {
    // CustomVector(size_type n, const T &val)
    {
        size_t N = 5;
        void *addr = malloc(sizeof(CustomVector<uint64_t>) + sizeof(uint64_t) * N);
        CustomVector<uint64_t> *vec_ptr = new(addr) CustomVector<uint64_t>(N, 1000);
        EXPECT_EQ(vec_ptr->capacity(), N);
        EXPECT_EQ(vec_ptr->size(), N);
        EXPECT_EQ(vec_ptr->_arr_offset, sizeof(CustomVector<uint64_t>));
        EXPECT_EQ(vec_ptr->front(), 1000);
        EXPECT_EQ(vec_ptr->back(), 1000);
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());

        vec_ptr->~CustomVector<uint64_t>();
        free(addr);
    }
    // CustomVector(size_type n, const T &val) n = 0
    {
        size_t N = 0;
        void *addr = malloc(sizeof(CustomVector<uint64_t>) + sizeof(uint64_t) * N);
        CustomVector<uint64_t> *vec_ptr = new(addr) CustomVector<uint64_t>(N, 1000);
        EXPECT_EQ(vec_ptr->capacity(), N);
        EXPECT_EQ(vec_ptr->size(), N);
        EXPECT_EQ(vec_ptr->_arr_offset, sizeof(CustomVector<uint64_t>));
        EXPECT_TRUE(vec_ptr->empty());
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());

        vec_ptr->~CustomVector<uint64_t>();
        free(addr);
    }
}

TEST_F(CustomVectorTest, test_construct_std_vec) {
    // CustomVector(const std::vector<T> &other)
    {
        std::vector<int32_t> other = {100, 200, 300, 400, 500};
        void *addr = malloc(sizeof(CustomVector<int32_t>) + sizeof(int32_t) * other.size());
        CustomVector<int32_t> *vec_ptr = new(addr) CustomVector<int32_t>(other);
        EXPECT_EQ(vec_ptr->capacity(), other.size());
        EXPECT_EQ(vec_ptr->size(), other.size());
        EXPECT_EQ(vec_ptr->_arr_offset, sizeof(CustomVector<int32_t>));
        EXPECT_EQ(vec_ptr->front(), 100);
        EXPECT_EQ(vec_ptr->back(), 500);
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());

        vec_ptr->~CustomVector<int32_t>();
        free(addr);
    }
    // CustomVector(const std::vector<T> &other) other is empty
    {
        std::vector<int32_t> other;
        void *addr = malloc(sizeof(CustomVector<int32_t>) + sizeof(int32_t) * other.size());
        CustomVector<int32_t> *vec_ptr = new(addr) CustomVector<int32_t>(other);
        EXPECT_EQ(vec_ptr->capacity(), other.size());
        EXPECT_EQ(vec_ptr->size(), other.size());
        EXPECT_EQ(vec_ptr->_arr_offset, sizeof(CustomVector<int32_t>));
        EXPECT_TRUE(vec_ptr->empty());
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());

        vec_ptr->~CustomVector<int32_t>();
        free(addr);
    }
}

TEST_F(CustomVectorTest, test_access) {
    // empty CustomVector. at() will throw. operator[] will core
    {
        void *addr = malloc(sizeof(CustomVector<int>));
        CustomVector<int> *vec_ptr = new(addr) CustomVector<int>;
        try {
            vec_ptr->at(0);
        } catch(std::exception &e) {
            std::cout << "catch! " << e.what() << std::endl;
        }
        EXPECT_EQ(vec_ptr->begin(), vec_ptr->end());

        vec_ptr->~CustomVector<int>();
        free(addr);
    }
    // not empty CustomVector. operator[]/at()/iterator
    {
        size_t N = 5;
        void *addr = malloc(sizeof(CustomVector<uint64_t>) + sizeof(uint64_t) * N);
        CustomVector<uint64_t> *vec_ptr = new(addr) CustomVector<uint64_t>({1, 2, 3, 4, 5});
        EXPECT_EQ(vec_ptr->front(), 1);
        EXPECT_EQ(vec_ptr->at(1), 2);
        EXPECT_EQ(vec_ptr->at(2), 3);
        EXPECT_EQ((*vec_ptr)[3], 4);
        EXPECT_EQ(vec_ptr->back(), 5);
        uint64_t val = 0;
        for (auto it = vec_ptr->begin(); it != vec_ptr->end(); ++it) {
            EXPECT_EQ(*it, ++val);
        }
        for (auto it = vec_ptr->cend() - 1; it >= vec_ptr->cbegin(); --it) {
            EXPECT_EQ(*it, val--);
        }

        vec_ptr->~CustomVector<uint64_t>();
        free(addr);
    }
}

TEST_F(CustomVectorTest, test_container_memsize) {
    // empty vec
    {
        void *addr = malloc(sizeof(CustomVector<Cat>));
        CustomVector<Cat> *vec_ptr = new(addr) CustomVector<Cat>;
        std::cout << vec_ptr->layout() << std::endl;
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());
        EXPECT_EQ(container_memsize(vec_ptr), sizeof(CustomVector<Cat>));

        vec_ptr->~CustomVector<Cat>();
        free(addr);
    }
    // vec not empty
    {
        std::vector<Cat> in = { {"miaoji"}, {"fm"}, {"mimi"}, {"dahuang"} };
        for (auto &cat : in) {
            std::cout << (void*)&cat << "\t" << cat.name << std::endl;
        }
        size_t exp_size = sizeof(CustomVector<Cat>) + sizeof(Cat) * in.size();
        void *addr = malloc(exp_size);
        CustomVector<Cat> *vec_ptr = new(addr) CustomVector<Cat>(in);
        std::cout << vec_ptr->layout() << std::endl;
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());
        EXPECT_EQ(container_memsize(vec_ptr), exp_size);

        vec_ptr->~CustomVector<Cat>();
        free(addr);
    }
}

TEST_F(CustomVectorTest, test_container_memsize_nested) {
    // empty vec
    {
        void *addr = malloc(sizeof(CustomVector<CustomVector<Cat> >));
        CustomVector<CustomVector<Cat> > *vec_ptr = new(addr) CustomVector<CustomVector<Cat> >;
        EXPECT_EQ(vec_ptr->capacity(), 0);
        EXPECT_EQ(vec_ptr->size(), 0);
        EXPECT_EQ(vec_ptr->_arr_offset, sizeof(CustomVector<CustomVector<Cat> >));
        std::cout << vec_ptr->layout() << std::endl;
        LEVIN_CDEBUG_LOG("%s", vec_ptr->layout().c_str());
        EXPECT_EQ(container_memsize(vec_ptr), sizeof(CustomVector<CustomVector<Cat> >));

        vec_ptr->~CustomVector<CustomVector<Cat> >();
        free(addr);
    }
    // vec not empty
    {
        std::vector<std::vector<Cat> > in = {
            {"miaoji", "fm"},
            {"mimi"},
            {"xiaomi", "dahuang", "kiki"}
        };
        for (auto &row : in) {
            for (auto &cat : row) {
                std::cout << (void*)&cat << "\t" << cat.name << std::endl;
            }
            std::cout << std::endl;
        }

        std::string name = "./nsvec_cat.dat";
        levin::SharedNestedVector<Cat, uint32_t>::Dump(name.c_str(), in);
        levin::SharedNestedVector<Cat, uint32_t> vec(name);
        if (vec.Init() != SC_RET_OK || (!vec.IsExist() && vec.Load() != SC_RET_OK)) {
            return;
        }
        for (auto row_it = vec.begin(); row_it != vec.end(); ++row_it) {
            for (auto it = row_it->cbegin(); it != row_it->cend(); ++it) {
                std::cout << (void*)it << "\t" << it->name << std::endl;
            }
            std::cout << std::endl;
        }
        size_t exp_size = sizeof(CustomVector<CustomVector<Cat, uint32_t>, size_t>) +
                          sizeof(CustomVector<Cat, uint32_t>) * in.size();
        exp_size += sizeof(Cat) * 6;
        EXPECT_EQ(container_memsize(vec._object), exp_size);

        vec.Destroy();
    }
}

}  // namespace levin

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
