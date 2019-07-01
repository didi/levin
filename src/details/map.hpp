#ifndef LEVIN_DETAILS_MAP_H
#define LEVIN_DETAILS_MAP_H

#include <sstream>
#include "shared_utils.h"
#include "vector.hpp"

namespace levin {

// @brief customized map which MUST be inplacement new at allocated address
template <class Key, class Value, class Compare = std::less<Key> >
class Map {
public:
    // @brief val: [ pair<key, value> ...]
    typedef CustomVector<std::pair<Key, Value>, size_t> impl_type;
    typedef typename impl_type::value_type     value_type;
    typedef Key                                key_type;
    typedef size_t                             size_type;
    typedef typename impl_type::iterator       iterator;
    typedef typename impl_type::const_iterator const_iterator;

    // @breif construct&destruct
    Map() {}
    ~Map() { /* do NOT delete[] */ }

    // @brief debugging
    std::string layout() const;

    size_type size() const { return _datas.size(); }
    bool empty() const { return _datas.size() == 0; }

    // @brief iterators
    iterator begin() { return _datas.begin(); }
    iterator end() { return _datas.end(); }
    const_iterator cbegin() const { return _datas.cbegin(); }
    const_iterator cend() const { return _datas.cend(); }

    const_iterator find(const Key& key) const {
        PairCompare<Key, Value, Compare> comp;
        auto first = std::lower_bound(_datas.cbegin(), _datas.cend(), key, comp);
        if (first == _datas.cend() || bool(comp(key, *first))) {
            return _datas.cend();
        }
        return first;
    }
    size_type count(const Key& key) const { return (find(key) == cend()? 0 : 1); }

    // @brief Do not inserts a new element when the key does not match the key of any element in the container
    // Throws an exception when it does not match
    const Value& operator[](const Key& key) const { return find(key)->second; }

    // @brief If key does not match the key of any element in the container
    // throws an out_of_range exception.
    const Value& at(const Key& key) const;

    const_iterator lower_bound(const Key& key) const {
        return std::lower_bound(
                _datas.cbegin(), _datas.cend(), key, PairCompare<Key, Value, Compare>());
    }

    const_iterator upper_bound(const Key& key) const {
        return std::upper_bound(
                _datas.cbegin(), _datas.cend(), key, PairCompare<Key, Value, Compare>());
    }

    std::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
        return std::equal_range(
                _datas.cbegin(), _datas.cend(), key, PairCompare<Key, Value, Compare>());
    }

    const impl_type& datas() const { return _datas; }

    void swap(Map<Key, Value, Compare> &other) { std::swap(_datas, other._datas); }
    bool operator ==(const Map<Key, Value, Compare> &other) const { return _datas == other._datas; }
    bool operator !=(const Map<Key, Value, Compare> &other) const { return !(*this == other); }
private:
    Map(const Map<Key, Value, Compare>&) = delete;
    Map(Map<Key, Value, Compare>&&) = delete;
    Map<Key, Value, Compare>& operator =(const Map<Key, Value, Compare>&) = delete;
    Map<Key, Value, Compare>& operator =(Map<Key, Value, Compare>&&) = delete;

private:
    impl_type _datas;
};

template <class Key, class Value, class Compare>
const Value& Map<Key, Value, Compare>::at(const Key& key) const {
    auto item = find(key);
    if (item == cend()) {
        throw std::out_of_range("accessed position out of range");
    }
    return item->second;
}

template <class Key, class Value, class Compare>
std::string Map<Key, Value, Compare>::layout() const {
    std::stringstream ss;
    ss << "Map this=[" << (void*)this << "]" << std::endl << _datas.layout();
    return ss.str();
}

template <class Key, class Value, class Compare>
inline size_t container_memsize(const Map<Key, Value, Compare> *object) {
    return (object->datas().empty() ? sizeof(*object) :
            (size_t)object->datas().cend() - (size_t)object);
}

}  // namespace levin

#endif  // LEVIN_DETAILS_MAP_H
