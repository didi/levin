#ifndef LEVIN_NESTED_HASHMAP_H
#define LEVIN_NESTED_HASHMAP_H

#include <sstream>
#include <unordered_map>
#include "shared_utils.h"
#include "details/vector.hpp"

namespace levin {

template<class Key, class Value>
class NHashIterator;

// @brief customized nested hashmap which MUST be inplacement new at allocated address
template <class Key, class Value, class Hash = std::hash<Key> >
class NestedHashMap {
public:
    // @brief impl by index_impl_type&data_impl_type combination
    // index: [ pair<key, pos> ]
    // data : [pos ->CustomVector<Value>]
    typedef CustomVector<CustomVector<std::pair<Key, size_t>, uint32_t>, size_t> index_impl_type;
    typedef CustomVector<CustomVector<Value, uint32_t>, size_t>                  data_impl_type;
    typedef typename index_impl_type::value_type   index_bucket_type;
    typedef typename data_impl_type::value_type    data_array_type;
    typedef typename index_bucket_type::value_type index_value_type;
    typedef typename index_bucket_type::size_type  index_value_size_type;
    typedef typename data_array_type::value_type   data_value_type;
    typedef typename data_array_type::size_type    data_value_size_type;
    // Tips: value_type diff from STD container, which is std::pair<Key, data_array_type>
    typedef std::pair<Key, data_array_type*>       value_type;
    typedef Key                                    key_type;
    typedef size_t                                 size_type;
    typedef NHashIterator<Key, Value> iterator;
    typedef NHashIterator<Key, Value> const_iterator;
    friend class NHashIterator<Key, Value>;

    // @breif construct&destruct
    NestedHashMap();
    explicit NestedHashMap(size_type n);
    ~NestedHashMap() { /* do NOT delete[] */ }

    // @brief debugging
    std::string layout();

    // @brief capacity
    bool empty() const { return _size == 0; }
    size_type size() const { return _size; }
    size_type bucket_size() const { return _bucket_size; }
    size_type index_size() const { return _index_size; }
    size_type data_size() const { return _data_size; }
    const index_impl_type& datas() const { return _index_datas; }

    // @brief element access (no range check)
    void swap(NestedHashMap<Key, Value, Hash> &other);

    // @brief iterators
    bool operator ==(const NestedHashMap<Key, Value, Hash> &other) const;
    bool operator !=(const NestedHashMap<Key, Value, Hash> &other) const {
        return !(*this == other);
    }

    iterator find(const Key& key);
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    size_type count(const Key& key) { return (find(key) == end()? 0 : 1); }

    const data_array_type& operator[](const Key& key) { return *(find(key)->second); }

    data_impl_type* data_array() {
        return static_cast<data_impl_type*>(
                (void *)((char *)&_index_datas + _index_size + sizeof(index_impl_type)));
    }
private:
    NestedHashMap(const NestedHashMap<Key, Value, Hash>&) = delete;
    NestedHashMap(NestedHashMap<Key, Value, Hash>&&) = delete;
    NestedHashMap<Key, Value, Hash>& operator=(const NestedHashMap<Key, Value, Hash>&) = delete;
    NestedHashMap<Key, Value, Hash>& operator=(NestedHashMap<Key, Value, Hash>&&) = delete;

private:
    size_type _size = 0;
    size_type _bucket_size = 0;
    size_type _index_size = 0;
    size_type _data_size = 0;
    index_impl_type _index_datas;
    // implicit member
    // data_impl_type _data_array;
};

template <class Key, class Value, class Hash>
NestedHashMap<Key, Value, Hash>::NestedHashMap() :
        _size(0),
        _bucket_size(0),
        _index_size(0),
        _data_size(0) {
}

template <class Key, class Value, class Hash>
NestedHashMap<Key, Value, Hash>::NestedHashMap(const size_type n) :
        _size(n),
        _bucket_size(getPrime(n)) {
}

template <class Key, class Value, class Hash>
typename NestedHashMap<Key, Value, Hash>::iterator NestedHashMap<Key, Value, Hash>::begin() {
    return NHashIterator<Key, Value>(
            0, _index_datas[0].begin(),
            _index_datas[_index_datas.size()-1].end(), data_array(), this);
}
template <class Key, class Value, class Hash>
typename NestedHashMap<Key, Value, Hash>::const_iterator NestedHashMap<Key, Value, Hash>::begin() const {
    return NHashIterator<Key, Value>(
            0, _index_datas[0].begin(),
            _index_datas[_index_datas.size()-1].end(), data_array(), this);
}

template <class Key, class Value, class Hash>
typename NestedHashMap<Key, Value, Hash>::iterator NestedHashMap<Key, Value, Hash>::end() {
    return NHashIterator<Key, Value>(
            -1, _index_datas[_index_datas.size()-1].end(),
            _index_datas[_index_datas.size()-1].end(), data_array(), this);
}

template <class Key, class Value, class Hash>
typename NestedHashMap<Key, Value, Hash>::const_iterator NestedHashMap<Key, Value, Hash>::end() const {
    return NHashIterator<Key, Value>(
            -1, _index_datas[_index_datas.size()-1].end(),
            _index_datas[_index_datas.size()-1].end(), data_array(), this);
}

template <class Key, class Value, class Hash>
typename NestedHashMap<Key, Value, Hash>::iterator NestedHashMap<Key, Value, Hash>::find(const Key& key) {
    uint32_t bucket_idx = Hash()(key) % _bucket_size;
    auto &bucket = _index_datas[bucket_idx];
    int32_t pos = binary_search(&bucket[0], bucket.size() - 1, key);
    if (pos >= 0 && pos < bucket.size() && bucket[pos].first == key) {
        return NHashIterator<Key, Value>(
                pos, bucket.begin() + pos,
                _index_datas[_index_datas.size()-1].end(), data_array(), this);
    }
    return end();
}

template <class Key, class Value, class Hash>
void NestedHashMap<Key, Value, Hash>::swap(NestedHashMap<Key, Value, Hash> &other) {
    std::swap(_size, other._size);
    std::swap(_bucket_size, other._bucket_size);
    std::swap(_index_size, other._index_size);
    std::swap(_data_size, other._data_size);
    std::swap(_index_datas, other._index_datas);
}

template <class Key, class Value, class Hash>
bool NestedHashMap<Key, Value, Hash>::operator ==(
        const NestedHashMap<Key, Value, Hash> &other) const {
    if (_size != other._size || _bucket_size != other._bucket_size) {
        return false;
    }
    return _index_datas == other._index_datas;
}

template <class Key, class Value, class Hash>
std::string NestedHashMap<Key, Value, Hash>::layout() {
    std::stringstream ss;
    ss << "NestedHashMap this=[" << (void*)this << "]" << std::endl
       << "[" << (void*)&_size << "]\t\t_size=" << _size << std::endl
       << "[" << (void*)&_bucket_size << "]\t\t_bucket_size=" << _bucket_size << std::endl
       << "[" << (void*)&_index_size << "]\t\t_index_size=" << _index_size << std::endl
       << "[" << (void*)&_data_size << "]\t\t_data_size=" << _data_size << std::endl
       << container_layout(&_index_datas) << std::endl
       << container_layout(data_array());
    return ss.str();
}

template <class Key, class Value, class Hash>
inline size_t container_memsize(const NestedHashMap<Key, Value, Hash> *object) {
    size_t container_size =
        (object->datas().empty() ? sizeof(*object) :
        (size_t)object->datas().back().cend() - (size_t)object);
    container_size += object->data_size();
    return container_size;
}

template<class Key, class Value>
class NHashIterator {
public:
    NHashIterator() {
        _pos = -1;
        _it = typename NestedHashMap<Key, Value>::index_bucket_type::iterator();
        _endit = typename NestedHashMap<Key, Value>::index_bucket_type::iterator();
        _hashmap = nullptr;
    }
    NHashIterator(
            int32_t pos,
            typename NestedHashMap<Key, Value>::index_bucket_type::iterator vit,
            typename NestedHashMap<Key, Value>::index_bucket_type::iterator endit,
            typename NestedHashMap<Key, Value>::data_impl_type* array,
            const NestedHashMap<Key, Value>* hashmap) :
            _pos(pos),
            _it(vit),
            _endit(endit),
            _array(array),
            _hashmap(hashmap) {
                _kvpair.first = _it->first;
                _kvpair.second = &((*_array)[_it->second]);
    }
    ~NHashIterator() {}
    const typename NestedHashMap<Key, Value>::value_type& operator*() const {
        if (_pos == -1) {
            throw std::out_of_range("accessed position out of range");
        }
        return _kvpair;
    }
    const typename NestedHashMap<Key, Value>::value_type* operator->() const {
        if (_pos == -1) {
            throw std::out_of_range("accessed position out of range");
        }
        return &_kvpair;
    }
    bool operator==(const NHashIterator& hsit) const {
        return (_hashmap == hsit._hashmap) && (_it == hsit._it);
    }
    bool operator!=(const NHashIterator& hsit) const {
        return !(operator == (hsit));
    }
    NHashIterator<Key, Value>& operator++();

private:
    int32_t _pos;
    typename NestedHashMap<Key, Value>::index_bucket_type::iterator _it;
    typename NestedHashMap<Key, Value>::index_bucket_type::iterator _endit;
    typename NestedHashMap<Key, Value>::value_type _kvpair;
    typename NestedHashMap<Key, Value>::data_impl_type* _array = nullptr;
    const NestedHashMap<Key, Value>* _hashmap = nullptr;
};

template<class Key, class Value>
NHashIterator<Key, Value>& NHashIterator<Key, Value>::operator++() {
    ++_it;
    if (_it != _endit) {
        _kvpair.first = _it->first;
        _kvpair.second = &((*_array)[_it->second]);
    }
    return *this;
}

}  // namespace levin

#endif  // LEVIN_NESTED_HASHMAP_H
