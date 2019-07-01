#ifndef LEVIN_SHARED_MAP_H
#define LEVIN_SHARED_MAP_H

#include <map>
#include "shared_base.hpp"
#include "details/map.hpp"
#include "svec.hpp"

namespace levin {

template <class Key,
          class Value,
          class Compare = std::less<Key>,
          class CheckFunc = levin::IntegrityChecker>
class SharedMap : public SharedBase {
public:
    typedef Map<Key, Value, Compare>                container_type;
    typedef typename container_type::impl_type      impl_type;
    typedef typename container_type::value_type     value_type;
    typedef typename container_type::iterator       iterator;
    typedef typename container_type::const_iterator const_iterator;
public:
    SharedMap(const std::string &name, const std::string group = "default", const int id = 1) :
        SharedBase(name, group, id, CheckFunc()),
        _object(nullptr) {
    }

    virtual int Init() override {
        return _init<container_type>(_object);
    }

    virtual int Load() override {
        return _load<container_type>(_object);
    }

    virtual bool Dump(const std::string &file) override;
    static bool Dump(const std::string &file, const std::map<Key, Value, Compare> &map);

    bool empty() const { return _object->empty(); }
    size_t size() const { return _object->size(); }
    iterator begin() { return _object->begin(); }
    iterator end() { return _object->end(); }
    const_iterator cbegin() const { return _object->cbegin(); }
    const_iterator cend() const { return _object->cend(); }

    const_iterator find(Key key) const { return _object->find(key); }
    size_t count(const Key& key) const { return _object->count(key); }
    const Value& at(const Key& key) const { return _object->at(key); }
    const Value& operator[](const Key& key) const { return find(key)->second; }

    const_iterator lower_bound(const Key& key) const { return _object->lower_bound(key); }
    const_iterator upper_bound(const Key& key) const { return _object->upper_bound(key); }
    std::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
        return _object->equal_range(key);
    }

    std::string layout() const {
        std::stringstream ss;
        ss << "SharedMap this=[" << (void*)this << "]";
        if (_info->_meta != nullptr) {
            ss << std::endl << *_info->_meta;
        }
        if (_object != nullptr) {
            ss << std::endl << _object->layout();
        }
        return ss.str();
    }
    //for debug
    size_t load_shm_size() const {
        if (_info && _object && !_object->datas().empty()) {
            return (size_t)_object->datas().cend() - (size_t)_info->_meta;
        }
        return 0;
    }
private:
    container_type *_object = nullptr;
};

template <class Key, class Value, class Compare, class CheckFunc>
bool SharedMap<Key, Value, Compare, CheckFunc>::Dump(const std::string &file) {
    return _bin2file(file, container_memsize(this->_object), _object);
}

template <class Key, class Value, class Compare, class CheckFunc>
bool SharedMap<Key, Value, Compare, CheckFunc>::Dump(
        const std::string &file, const std::map<Key, Value, Compare> &mp) {
    std::vector<value_type> vec(mp.cbegin(), mp.cend());
    return SharedVector<value_type>::Dump(file, vec, typeid(container_type).hash_code());
}

template <class Key, class Value>
inline std::ostream& operator<<(std::ostream &os, const SharedMap<Key, Value> &map) {
    return os << map.layout();
}
 
}  // namespace levin

#endif  // LEVIN_SHARED_MAP_H
