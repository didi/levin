#ifndef LEVIN_DETAILS_HASHSET_H
#define LEVIN_DETAILS_HASHSET_H

#include <sstream>
#include <unordered_set>
#include "shared_utils.h"
#include "vector.hpp"

namespace levin {

// @brief customized hashset which MUST be inplacement new at allocated address
template <class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key> >
class HashSet {
public:
    // @brief typedefs
    // key: hash buckets idx  val: [ key ]
    typedef CustomVector<CustomVector<Key, uint32_t>, size_t> impl_type;
    typedef typename impl_type::value_type       bucket_type;
    typedef typename bucket_type::value_type     value_type;
    typedef typename bucket_type::size_type      value_size_type;
    typedef Key                                  key_type;
    typedef size_t                               size_type;
    typedef typename bucket_type::iterator       iterator;
    typedef typename bucket_type::const_iterator const_iterator;

    // @breif constructor&destructor
    HashSet() : _size(0), _bucket_count(1), _datas(_bucket_count) {
    }
    ~HashSet() { /* do NOT delete[] */ }

    // @brief debugging
    std::string layout() const;

    // @brief Capacity
    bool empty() const { return _size == 0; }
    size_type size() const { return _size; }

    // @brief Element lookup
    const_iterator find(const Key& k) const;
    size_type count(const Key& k) const { return (find(k) == cend() ? 0 : 1); }

    // @brief Buckets
    size_type bucket_count() const { return _bucket_count; }
    // @brief no range check, as std unordered_set
    size_type bucket_size(size_type n) const { return _datas[n].size(); }
    const impl_type& datas() const { return _datas; }

    // @brief Iterators
    iterator begin() { return _datas.front().begin(); }
    iterator end() { return _datas.back().end(); }
    const_iterator cbegin() const { return _datas.front().cbegin(); }
    const_iterator cend() const { return _datas.back().cend(); }

    // @brief Modifiers
    void swap(HashSet<Key, Hash, Pred> &other);

    bool operator ==(const HashSet<Key, Hash, Pred> &other) const;
    bool operator !=(const HashSet<Key, Hash, Pred> &other) const { return !(*this == other); }

private:
    HashSet(const HashSet<Key, Hash, Pred>&) = delete;
    HashSet(HashSet<Key, Hash, Pred>&&) = delete;
    HashSet<Key, Hash, Pred>& operator =(const HashSet<Key, Hash, Pred>&) = delete;
    HashSet<Key, Hash, Pred>& operator =(HashSet<Key, Hash, Pred>&&) = delete;

private:
    size_type _size = 0;
    size_type _bucket_count = 0;
    impl_type _datas;
};

template <class Key, class Hash, class Pred>
LEVIN_INLINE typename HashSet<Key, Hash, Pred>::const_iterator HashSet<Key, Hash, Pred>::find(
        const Key& k) const {
    const auto &bucket = _datas[Hash()(k) % _bucket_count];
    Pred pred;
    for (const auto &item : bucket) {
        if (pred(item, k)) {
            return &item;
        }
    }
    return cend();
}

template <class Key, class Hash, class Pred>
void HashSet<Key, Hash, Pred>::swap(HashSet<Key, Hash, Pred> &other) {
    std::swap(_size, other._size);
    std::swap(_bucket_count, other._bucket_count);
    std::swap(_datas, other._datas);
}

template <class Key, class Hash, class Pred>
bool HashSet<Key, Hash, Pred>::operator ==(const HashSet<Key, Hash, Pred> &other) const {
    if (_size != other._size || _bucket_count != other._bucket_count) {
        return false;
    }
    return _datas == other._datas;
}

template <class Key, class Hash, class Pred>
std::string HashSet<Key, Hash, Pred>::layout() const {
    std::stringstream ss;
    ss << "HashSet this=[" << (void*)this << "]" << std::endl
       << "[" << (void*)&_size << "]\t\t_size=" << _size << std::endl
       << "[" << (void*)&_bucket_count << "]\t\t_bucket_count=" << _bucket_count << std::endl
       << container_layout(&_datas);
    return ss.str();
}

template <class Key, class Hash, class Pred>
inline size_t container_memsize(const HashSet<Key, Hash, Pred> *object) {
    return (object->datas().empty() ?
            sizeof(*object) :
            (size_t)object->datas().back().cend() - (size_t)object);
}

}  // namespace levin

#endif  // LEVIN_DETAILS_HASHSET_H
