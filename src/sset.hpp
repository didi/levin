#ifndef LEVIN_SHARED_SET_H
#define LEVIN_SHARED_SET_H

#include <set>
#include <vector>
#include "details/set.hpp"
#include "svec.hpp"

namespace levin {

template <class Key, class Compare = std::less<Key>, class CheckFunc = levin::IntegrityChecker>
class SharedSet : public SharedBase {
public:
    typedef CustomSet<Key, Compare>             container_type;
    typedef typename container_type::impl_type  impl_type;
    typedef typename container_type::value_type value_type;

    SharedSet() : SharedBase(), _object(nullptr) {
    }
    SharedSet(const std::string &name, const std::string group = "default", const int id = 1) :
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
    static bool Dump(const std::string &file, const std::set<Key, Compare> &st);

    bool empty() const { return _object->empty(); }
    size_t size() const { return _object->size(); }
    const Key* cbegin() const { return _object->cbegin(); }
    const Key* cend() const { return _object->cend(); }
    // ret type const refrence for reminder readonly
    const Key* begin() const { return _object->begin(); }
    const Key* end() const { return _object->end(); }
    const Key* find(const Key& val) const { return _object->find(val); }
    size_t count(const Key& val) const { return _object->count(val); }
    const Key* lower_bound(const Key& val) const { return _object->lower_bound(val); }
    const Key* upper_bound(const Key& val) const { return _object->upper_bound(val); }
    std::pair<const Key*, const Key*> equal_range(const Key& val) const {
        return _object->equal_range(val);
    }

    std::string layout() const;

protected:
    container_type *_object = nullptr;
};

template <class Key, class Compare, class CheckFunc>
bool SharedSet<Key, Compare, CheckFunc>::Dump(const std::string &file) {
    return this->_bin2file(file, container_memsize(_object), _object);
}

template <class Key, class Compare, class CheckFunc>
bool SharedSet<Key, Compare, CheckFunc>::Dump(
        const std::string &file, const std::set<Key, Compare> &st) {
    std::vector<value_type> vec(st.cbegin(), st.cend());
    return SharedVector<value_type>::Dump(file, vec, typeid(container_type).hash_code());
}

template <class Key, class Compare, class CheckFunc>
std::string SharedSet<Key, Compare, CheckFunc>::layout() const {
    std::stringstream ss;
    ss << "SharedSet this=[" << (void*)this << "]";
    if (_info->_meta != nullptr) {
        ss << std::endl << *_info->_meta;
    }
    if (_object != nullptr) {
        ss << std::endl << _object->layout();
    }
    return ss.str();
}

}  // namespace levin

#endif  // LEVIN_SHARED_SET_H
