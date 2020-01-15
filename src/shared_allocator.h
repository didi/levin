#ifndef LEVIN_SHARED_ALLOCATOR_H
#define LEVIN_SHARED_ALLOCATOR_H

#include <stdexcept>
#include <typeinfo>
#include <boost/noncopyable.hpp>
#include "levin_logger.h"

namespace levin {

class SharedAllocator : public boost::noncopyable {
public:
    SharedAllocator(const void *base_addr, const size_t capacity = 100000000000) :
            _base_addr(base_addr),
            _capacity(capacity),
            _used_size(0) {
    }
    virtual ~SharedAllocator() {
    }

    void Reset() {
        _used_size = 0;
    }

    // @brief offset address within a mapped region, NO new operator (maybe THROW)
    template <typename T>
    T* Address() {
        void *addr = allocate(sizeof(T));
        if (addr == nullptr) {
            LEVIN_CWARNING_LOG("out of memory, Address %s fail!", typeid(T).name());
            throw std::runtime_error("out of memory, bad read!");
        }
        return static_cast<T*>(addr);
    }

    // @brief allocate and inplacement new, construct with params (maybe THROW)
    template <typename T, typename... Args>
    T* Construct(Args&&... args) {
        void *addr = allocate(sizeof(T));
        if (addr == nullptr) {
            LEVIN_CWARNING_LOG("out of memory, Construct %s fail!", typeid(T).name());
            throw std::runtime_error("out of memory, bad alloc!");
        }
        T *p = new(addr) T(std::forward<Args>(args)...);
        return p;
    }

    // @brief allocate and inplacement new array (maybe THROW)
    template <typename T>
    T* ConstructN(std::ptrdiff_t size) {
        void *addr = allocate(size * sizeof(T));
        if (addr == nullptr) {
            LEVIN_CWARNING_LOG("out of memory, ConstructN %ld %s fail!", size, typeid(T).name());
            throw std::runtime_error("out of memory, bad alloc!");
        }
        T *p = new(addr) T[size];
        return p;
    }

    size_t used_size() const {
        return _used_size;
    }

    bool OutOfRange(void *addr, size_t len) const {
        size_t lower_bound = (size_t)_base_addr;
        size_t upper_bound = (size_t)_base_addr + _capacity;
        void *upper_addr = (void*)((size_t)addr + len);
        //if ((size_t)addr < lower_bound || (size_t)upper_addr > upper_bound) {
        // check bound strictly
        if ((size_t)addr != lower_bound || (size_t)upper_addr != upper_bound) {
            LEVIN_CWARNING_LOG("[%p,%p) out of range [%p,%p)",
                    addr, upper_addr, (void*)lower_bound, (void*)upper_bound);
            return true;
        }
        return false;
    }
    bool OutOfRange(void *upper_addr) const {
        size_t upper_bound = (size_t)_base_addr + _capacity;
        if ((size_t)upper_addr != upper_bound) {
            LEVIN_CWARNING_LOG("%p out of upper bound %p", upper_addr, (void*)upper_bound);
            return true;
        }
        return false;
    }

    static size_t Allocsize(size_t size) {
        static const size_t ALIGN_BYTES = 8;
        return (size + ALIGN_BYTES - 1) & (~(ALIGN_BYTES - 1));
    }

private:
    void *allocate(size_t size) {
        size_t alloc_size = Allocsize(size);

        if (_used_size + alloc_size > _capacity) {
            LEVIN_CWARNING_LOG("allocate fail.used=%ld, alloc=%ld, capacity=%ld",
                    _used_size, alloc_size, _capacity);
            return nullptr;
        }
        void *addr = (char*)_base_addr + _used_size;
        _used_size += alloc_size;
        return addr;
    }

private:
    const void *_base_addr;
    const size_t _capacity;
    size_t _used_size;
};

}  // namespace levin

#endif  // LEVIN_SHARED_ALLOCATOR_H
