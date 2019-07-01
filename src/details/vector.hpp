#ifndef LEVIN_DETAILS_VECTOR_H
#define LEVIN_DETAILS_VECTOR_H

#include <iterator>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <typeinfo>
#include <type_traits>
#include "shared_allocator.h"

namespace levin {

// @brief customized vector which MUST be inplacement new at allocated address
// eg. shm/mmap region or sufficient malloced heap region
// which arrange object layout SEQUENTIALLY for shared inter-process
// which SHOULD be specified fixed capacity when construct
// which NOT support reallocate, NOT support copy construct or assign
template <class T, class SizeType = std::size_t>
class CustomVector {
public:
    // @brief typedefs
    typedef T         value_type;
    typedef T&        reference;
    typedef const T&  const_reference;
    typedef T*        pointer;
    typedef const T*  const_pointer;
    typedef T*        iterator;
    typedef const T*  const_iterator;
    typedef ptrdiff_t difference_type;
    typedef SizeType  size_type;
    // @brief Generally, class contains pointer member will implement destruct func explicitly
    // it's a rougth check solution to avoid some(NOT all) misusage
    // eg. CustomVector<std::string<int> >  —— error
    //     CustomVector<int>                —— ok
    //     CustomVector<CustomVector<int> > —— ok
    static_assert(
            std::is_trivially_destructible<value_type>::value,
            "illegal type, T maybe contains pointer member");
    // @brief SizeType should be unsigned integral types and should specialize numeric_limits
    static_assert(
            std::is_unsigned<size_type>::value,
            "illegal size type, SizeType MUST be unsigned integer types");

    // @breif construct&destruct
    CustomVector();
    explicit CustomVector(size_type n);
    explicit CustomVector(std::initializer_list<T> lst);
    CustomVector(size_type n, const T &val);
    CustomVector(size_type n, SharedAllocator &alloc);
    explicit CustomVector(const std::vector<T> &other);
    // no explicit destruct for satisfy is_trivially_destructible
    // ~CustomVector() { /* do NOT delete[] */ }

    // @brief for debugging
    std::string layout() const;

    // @brief capacity
    bool empty() const { return _size == 0; }
    size_type size() const { return _size; }
    size_type capacity() const { return _size; }

    // @brief element access (no range check)
    reference operator [](size_type idx) { return _array()[idx]; }
    const_reference operator [](size_type idx) const { return _array()[idx]; }
    reference front() { return _array()[0]; }
    const_reference front() const { return _array()[0]; }
    reference back() { return _array()[_size - 1]; }
    const_reference back() const { return _array()[_size - 1]; }

    // @brief element access (with range check, maybe THROW)
    reference at(size_type pos);
    const_reference at(size_type pos) const;

    // @brief data access
    T* data() { return _array(); }
    const T* data() const { return _array(); }

    // NOT support modifiers NOW
    // @brief modifiers (with range check, maybe THROW)
    //template <class... Args> void emplace_back(Args&&... args);
    //void push_back(const T &val);
    //void push_back(T &&val);
    //void pop_back();
    //void clear();
    void swap(CustomVector<T, SizeType> &other);

    // @brief iterators
    iterator begin() { return _array(); }
    const_iterator begin() const { return _array(); }
    const_iterator cbegin() const { return _array(); }
    iterator end() { return _array() + _size; }
    const_iterator end() const { return _array() + _size; }
    const_iterator cend() const { return _array() + _size; }

    bool operator ==(const CustomVector<T, SizeType> &other) const;
    bool operator !=(const CustomVector<T, SizeType> &other) const { return !(*this == other); }

    template <class X, class Y> friend class SharedVector;

private:
    T *_array() const { return (T*)((std::size_t)this + _arr_offset); }

    CustomVector(const CustomVector<T, SizeType>&) = delete;
    CustomVector(CustomVector<T, SizeType>&&) = delete;
    CustomVector<T>& operator =(const CustomVector<T, SizeType>&) = delete;
    CustomVector<T>& operator =(CustomVector<T, SizeType>&&) = delete;

private:
    // NOT support _capacity and related method for space efficiency
    size_type _size = 0;
    size_type _arr_offset = 0;
};

template <class T, class SizeType>
CustomVector<T, SizeType>::CustomVector() :
        _size(0),
        _arr_offset(sizeof(*this)) {
}

template <class T, class SizeType>
CustomVector<T, SizeType>::CustomVector(typename CustomVector<T, SizeType>::size_type n) :
        _size(n),
        _arr_offset(0) {
    // default arrange object layout sequentially, do NOT be overlapped
    // eg. place array at address (size_t)this + sizeof(CustomVector<T>)
    SharedAllocator alloc((void*)(this + 1));
    T *array = alloc.template ConstructN<T>(n);
    _arr_offset = (size_t)array - (size_t)this;
}

template <class T, class SizeType>
CustomVector<T, SizeType>::CustomVector(
        typename CustomVector<T, SizeType>::size_type n, SharedAllocator &alloc) :
        _size(n),
        _arr_offset(0) {
    T *array =  alloc.template ConstructN<T>(n);
    _arr_offset = (size_t)array - (size_t)this;
}

template <class T, class SizeType>
CustomVector<T, SizeType>::CustomVector(
        typename CustomVector<T, SizeType>::size_type n, const T &value) :
        _size(0),
        _arr_offset(0) {
    SharedAllocator alloc((void*)(this + 1));
    T *array = alloc.template ConstructN<T>(n);
    _arr_offset = (size_t)array - (size_t)this;
    for (size_type i = 0; i < n; ++i) {
        array[_size++] = value;
    }
}

template <class T, class SizeType>
CustomVector<T, SizeType>::CustomVector(std::initializer_list<T> lst) :
        _size(0),
        _arr_offset(0) {
    SharedAllocator alloc((void*)(this + 1));
    T *array = alloc.template ConstructN<T>(lst.size());
    _arr_offset = (size_t)array - (size_t)this;
    for (auto &item : lst) {
        array[_size++] = item;
    }
}

template <class T, class SizeType>
CustomVector<T, SizeType>::CustomVector(const std::vector<T> &other) :
        _size(0),
        _arr_offset(0) {
    SharedAllocator alloc((void*)(this + 1));
    T *array = alloc.template ConstructN<T>(other.size());
    _arr_offset = (size_t)array - (size_t)this;
    for (const auto &item : other) {
        array[_size++] = item;
    }
}

template <class T, class SizeType>
inline typename CustomVector<T, SizeType>::reference CustomVector<T, SizeType>::at(size_type pos) {
    if (pos >= _size) {
        throw std::out_of_range("accessed position out of range");
    }
    return _array()[pos];
}

template <class T, class SizeType>
inline typename CustomVector<T, SizeType>::const_reference CustomVector<T, SizeType>::at(
        size_type pos) const {
    if (pos >= _size) {
        throw std::out_of_range("accessed position out of range");
    }
    return _array()[pos];
}
#if 0
template <class T>
template <class... Args>
void CustomVector<T>::emplace_back(Args&&... args) {
    if (_size == _capacity) {
        throw std::out_of_range("exceed capacity");
    }
    _array()[_size] = std::move(T(std::forward<Args>(args)...));
    ++_size;
}

template <class T>
void CustomVector<T>::push_back(const T &val) {
    if (_size == _capacity) {
        throw std::out_of_range("exceed capacity");
    }
    _array()[_size] = val;
    ++_size;
}

template <class T>
void CustomVector<T>::push_back(T &&val) {
    if (_size == _capacity) {
        throw std::out_of_range("exceed capacity");
    }
    _array()[_size] = std::move(val);
    ++_size;
}

template <class T>
void CustomVector<T>::pop_back() {
    if (_size == 0) {
        throw std::out_of_range("empty CustomVector");
    }
    --_size;
    _array()[_size].~T();
}

template <class T>
void CustomVector<T>::clear() {
    for (size_type i = 0; i < _size; ++i) {
        _array()[i].~T();
    }
    _size = 0;
}
#endif

template <class T, class SizeType>
void CustomVector<T, SizeType>::swap(CustomVector<T, SizeType> &other) {
    std::swap(_size, other._size);
    std::swap(_arr_offset, other._arr_offset);
}

template <class T, class SizeType>
bool CustomVector<T, SizeType>::operator ==(const CustomVector<T, SizeType> &other) const {
    if (_size != other._size) {
        return false;
    }
    for (size_type i = 0; i < _size; ++i) {
        if (_array()[i] != other._array()[i]) {
            return false;
        }
    }
    return true;
}

template <class T, class SizeType>
std::string CustomVector<T, SizeType>::layout() const {
    std::stringstream ss;
    ss << "CustomVector this=[" << (void*)this << "]" << std::endl
       << "[" << (void*)&_size << "]\t\t_size=" << _size << std::endl
       << "[" << (void*)&_arr_offset << "]\t\t_arr_offset=0x" << std::hex << _arr_offset;
    if (_size != 0) {
        ss << std::endl << "[" << (void*)cbegin() << "," << (void*)cend()
           << ")\tT=" << typeid(T).name();
    }
    return ss.str();
}

template <class T, class SizeType>
inline size_t container_memsize(const CustomVector<T, SizeType> *object) {
    return (size_t)object->cend() - (size_t)object;
}

template <class T, class SizeType>
inline size_t container_memsize(const CustomVector<CustomVector<T, SizeType> > *object) {
    return (object->empty() ? sizeof(*object) : (size_t)object->back().cend() - (size_t)object);
};

template <class T, class SizeType>
inline std::string container_layout(const CustomVector<CustomVector<T, SizeType> > *object) {
    std::stringstream ss;
    ss << "Nested CustomVector this=[" << (void*)object << "]";

    static size_t LAYOUT_MAX_ROW_COUNT = 3;
    if (object != nullptr) {
        ss << std::endl << object->layout();
        // truncate while too long
        auto row = object->cbegin();
        for (size_t row_cnt = 0; row != object->cend() && row_cnt < LAYOUT_MAX_ROW_COUNT;
                ++row, ++row_cnt) {
            ss << std::endl << row->layout();
        }
        if (row != object->cend()) {
            auto last_one = std::prev(object->cend());
            if (row != last_one) {
                ss << std::endl << "...";
            }
            ss << std::endl << last_one->layout();
        }
    }
    return ss.str();
}

}  // namespace levin

#endif  // LEVIN_DETAILS_VECTOR_H
