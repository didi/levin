#ifndef LEVIN_DETAILS_HASHMAP_H
#define LEVIN_DETAILS_HASHMAP_H

#include <sstream>
#include <unordered_map>
#include "shared_utils.h"
#include "vector.hpp"

namespace levin {

template<class Key, class Value>
class HashIterator;

// @brief customized hashmap which MUST be inplacement new at allocated address
template <class Key, class Value, class Hash = std::hash<Key> >
class HashMap {
public:
    // @brief typedefs
    // key: hash buckets idx
    // val: [ pair<key, value> ]
    typedef CustomVector<CustomVector<std::pair<Key, Value>, uint32_t>, size_t> impl_type;
    typedef typename impl_type::value_type   bucket_type;
    typedef typename bucket_type::value_type value_type;
    typedef typename bucket_type::size_type  value_size_type;
    typedef Key                              key_type;
    typedef size_t                           size_type;
    typedef HashIterator<Key, Value>         iterator;
    typedef HashIterator<Key, Value>         const_iterator;
    friend class HashIterator<Key, Value>;

    // @breif construct&destruct
    HashMap();
    explicit HashMap(size_type n);
    ~HashMap() { /* do NOT delete[] */ }

    // @brief debugging
    std::string layout() const;

    // @brief capacity
    bool empty() const { return _size == 0; }
    size_type size() const { return _size; }
    size_type bucket_size() const { return _bucket_size; }
    const impl_type& datas() const { return _datas; }

    // @brief element access (no range check)
    void swap(HashMap<Key, Value, Hash> &other);

    // @brief iterators
    bool operator ==(const HashMap<Key, Value, Hash> &other) const;
    bool operator !=(const HashMap<Key, Value, Hash> &other) const { return !(*this == other); }

    iterator find(const Key& key);
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    size_type count(const Key& key);

    // @brief Do not inserts a new element when the key does not match the key of any element in the container
    // Throws an exception when it does not match
    const Value& operator[](const Key& key);

    // If key does not match the key of any element in the container, the function throws an out_of_range exception.
    const Value& at (const Key& key);
private:
    HashMap(const HashMap<Key, Value, Hash>&) = delete;
    HashMap(HashMap<Key, Value, Hash>&&) = delete;
    HashMap<Key, Value, Hash>& operator =(const HashMap<Key, Value, Hash>&) = delete;
    HashMap<Key, Value, Hash>& operator =(HashMap<Key, Value, Hash>&&) = delete;

private:
    size_type _size = 0;
    size_type _bucket_size = 0;
    impl_type _datas;
}; // HashMap

template <class Key, class Value, class Hash>
HashMap<Key, Value, Hash>::HashMap() :
        _size(0),
        _bucket_size(0) {
}

template <class Key, class Value, class Hash>
HashMap<Key, Value, Hash>::HashMap(const size_type n) :
        _size(n),
        _bucket_size(getPrime(n)) {
}

template <class Key, class Value, class Hash>
typename HashMap<Key, Value, Hash>::iterator HashMap<Key, Value, Hash>::begin() {
    return HashIterator<Key, Value>(0, _datas[0].begin(), this);
}
template <class Key, class Value, class Hash>
typename HashMap<Key, Value, Hash>::const_iterator HashMap<Key, Value, Hash>::begin() const {
    return HashIterator<Key, Value>(0, _datas[0].begin(), this);
}

template <class Key, class Value, class Hash>
typename HashMap<Key, Value, Hash>::iterator HashMap<Key, Value, Hash>::end() {
    return HashIterator<Key, Value>(-1, _datas[_datas.size()-1].end(), this);
}
template <class Key, class Value, class Hash>
typename HashMap<Key, Value, Hash>::const_iterator HashMap<Key, Value, Hash>::end() const {
    return HashIterator<Key, Value>(-1, _datas[_datas.size()-1].end(), this);
}

template <class Key, class Value, class Hash>
typename HashMap<Key, Value, Hash>::iterator HashMap<Key, Value, Hash>::find(const Key& key) {
    uint32_t bucket_idx = Hash()(key) % _bucket_size;
    auto &bucket = _datas[bucket_idx];
    int32_t pos = binary_search(&bucket[0], bucket.size() - 1, key);
    if (pos >= 0 && pos < bucket.size() && bucket[pos].first == key) {
        return HashIterator<Key, Value>(pos, bucket.begin() + pos, this);
    }
    return end();
}

template<class Key, class Value, class Hash>
typename HashMap<Key, Value, Hash>::size_type HashMap<Key, Value, Hash>::count(const Key& key) {
    return find(key) == end()? 0 : 1;
}

template<class Key, class Value, class Hash>
const Value& HashMap<Key, Value, Hash>::operator[](const Key& key) {
    return find(key)->second;
}

template<class Key, class Value, class Hash>
const Value& HashMap<Key, Value, Hash>::at(const Key& key) {
    return find(key)->second;
}

template <class Key, class Value, class Hash>
void HashMap<Key, Value, Hash>::swap(HashMap<Key, Value, Hash> &other) {
    std::swap(_size, other._size);
    std::swap(_bucket_size, other._bucket_size);
    std::swap(_datas, other._datas);
}

template <class Key, class Value, class Hash>
bool HashMap<Key, Value, Hash>::operator ==(const HashMap<Key, Value, Hash> &other) const {
    if (_size != other._size || _bucket_size != other._bucket_size) {
        return false;
    }
    return _datas == other._datas;
}

template <class Key, class Value, class Hash>
std::string HashMap<Key, Value, Hash>::layout() const {
    std::stringstream ss;
    ss << "HashMap this=[" << (void*)this << "]" << std::endl
       << "[" << (void*)&_size << "]\t\t_size=" << _size << std::endl
       << "[" << (void*)&_bucket_size << "]\t\t_bucket_size=" << _bucket_size << std::endl
       << container_layout(&_datas);
    return ss.str();
}

template <class Key, class Value, class Hash>
inline size_t container_memsize(const HashMap<Key, Value, Hash> *object) {
    return (object->datas().empty() ? sizeof(*object) :
            (size_t)object->datas().back().cend() - (size_t)object);
}

template<class Key, class Value>
class HashIterator {
public:
    HashIterator() {
        _pos = -1;
        _it = typename HashMap<Key, Value>::bucket_type::iterator();
        _hashmap = nullptr;
    }
    HashIterator(int32_t pos,
            typename HashMap<Key, Value>::bucket_type::iterator vit,
            const HashMap<Key, Value>* hashmap) :
         _pos(pos),
         _it(vit),
         _hashmap(hashmap) {
    }
    ~HashIterator() {}
    const typename HashMap<Key, Value>::value_type& operator*() const {
        if (_pos == -1) {
            throw std::out_of_range("accessed position out of range");
        }
        return *_it;
    }
    const typename HashMap<Key, Value>::value_type* operator->() const {
        if (_pos == -1) {
            throw std::out_of_range("accessed position out of range");
        }
        return _it;
    }
    bool operator==(const HashIterator& hsit) const {
        return (_hashmap == hsit._hashmap) && (_it == hsit._it);
    }
    bool operator!=(const HashIterator& hsit) const {
        return !(operator==(hsit));
    }
    HashIterator<Key, Value>& operator++() {
        ++_it;
        return *this;
    }
private:
    int32_t _pos;
    typename HashMap<Key, Value>::bucket_type::iterator _it;
    const HashMap<Key, Value>* _hashmap;
};  // class HashIterator

}  // namespace levin

#endif  // LEVIN_DETAILS_HASHMAP_H
