#ifndef LEVIN_DETAILS_SET_H
#define LEVIN_DETAILS_SET_H

#include <sstream>
#include <set>
#include "shared_utils.h"
#include "vector.hpp"

namespace levin {

// @brief customized set which MUST be inplacement new at allocated address
template <class Key, class Compare = std::less<Key> >
class CustomSet {
public:
    // @brief typedefs
    typedef CustomVector<Key, size_t>          impl_type;
    typedef typename impl_type::value_type     value_type;
    typedef Key                                key_type;
    typedef size_t                             size_type;
    typedef typename impl_type::iterator       iterator;
    typedef typename impl_type::const_iterator const_iterator;

    // @breif constructor&destructor
    CustomSet() {
    }
    ~CustomSet() { /* do NOT delete[] */ }

    // @brief Capacity
    bool empty() const { return _data.empty(); }
    size_type size() const { return _data.size(); }

    // @brief Operations
    LEVIN_INLINE const_iterator find(const Key& k) const {
        Compare comp;
        auto first = std::lower_bound(_data.cbegin(), _data.cend(), k, comp);
        if (first == _data.cend() || bool(comp(k, *first))) {
            return _data.cend();
        }
        return first;
    }
    size_type count(const Key& k) const { return (find(k) == _data.cend() ? 0 : 1); }
    const_iterator lower_bound(const Key& k) const {
        return std::lower_bound(_data.cbegin(), _data.cend(), k, Compare());
    }
    const_iterator upper_bound(const Key& k) const {
        return std::upper_bound(_data.cbegin(), _data.cend(), k, Compare());
    }
    std::pair<const_iterator, const_iterator> equal_range(const Key& k) const {
        return std::equal_range(_data.cbegin(), _data.cend(), k, Compare());
    }
    const impl_type& data() const { return _data; }

    // @brief Iterators
    iterator begin() { return _data.begin(); }
    iterator end() { return _data.end(); }
    const_iterator cbegin() const { return _data.cbegin(); }
    const_iterator cend() const { return _data.cend(); }

    // @brief debugging
    std::string layout() const {
        std::stringstream ss;
        ss << "CustomSet this=[" << (void*)this << "]" << std::endl << _data.layout();
        return ss.str();
    }

    // @brief Modifiers
    void swap(CustomSet<Key, Compare> &other) { std::swap(_data, other._data); }

    bool operator ==(const CustomSet<Key, Compare> &other) const { return _data == other._data; }
    bool operator !=(const CustomSet<Key, Compare> &other) const { return !(*this == other); }

private:
    CustomSet(const CustomSet<Key, Compare>&) = delete;
    CustomSet(CustomSet<Key, Compare>&&) = delete;
    CustomSet<Key, Compare>& operator =(const CustomSet<Key, Compare>&) = delete;
    CustomSet<Key, Compare>& operator =(CustomSet<Key, Compare>&&) = delete;

private:
    impl_type _data;
};

template <class Key, class Compare>
inline size_t container_memsize(const CustomSet<Key, Compare> *object) {
    return container_memsize(&object->data());
}

}  // namespace levin

#endif  // LEVIN_DETAILS_SET_H
