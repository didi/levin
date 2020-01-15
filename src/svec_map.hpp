#ifndef LEVIN_SHARED_VECTOR_NESTED_MAP_HPP
#define LEVIN_SHARED_VECTOR_NESTED_MAP_HPP

#include <map>
#include "shared_base.hpp"
#include "details/map.hpp"
#include "svec.hpp"

namespace levin {

// SharedNestedMap implement as SharedVector<Map<Key, Value> >
template <class Key,
          class Value,
          class Compare = std::less<Key>,
          class SizeType = uint32_t,
          class Mem = levin::SharedMemory,
          class CheckFunc = levin::IntegrityChecker>
class SharedNestedMap : public SharedVector<Map<Key, Value, Compare, SizeType>, Mem, CheckFunc> {
public:
    typedef CustomVector<Map<Key, Value, Compare, SizeType>, std::size_t> container_type;
    typedef typename container_type::value_type row_value_type;
    typedef typename row_value_type::value_type col_value_type;
    typedef typename container_type::size_type  row_size_type;
    typedef typename row_value_type::size_type  col_size_type;

    SharedNestedMap(const std::string &name, const std::string group = "default", const int id = 1) :
            SharedVector<Map<Key, Value, Compare, SizeType>, Mem, CheckFunc>(name, group, id) {
    }

    static bool Dump(const std::string &file, const std::vector<std::map<Key, Value, Compare> > &vecmap);
    std::string layout() const;
};

template <class Key, class Value, class Compare, class SizeType, class Mem, class CheckFunc>
bool SharedNestedMap<Key, Value, Compare, SizeType, Mem, CheckFunc>::Dump(
        const std::string &file, const std::vector<std::map<Key, Value, Compare> > &vecmap) {
    std::vector<std::vector<std::pair<Key, Value> > > datas;
    datas.reserve(vecmap.size());
    for (const auto &map : vecmap) {
        datas.emplace_back(map.begin(), map.end());
    }
    return SharedNestedVector<col_value_type, col_size_type>::Dump(
            file, datas, typeid(container_type).hash_code());
}

template <class Key, class Value, class Compare, class SizeType, class Mem, class CheckFunc>
std::string SharedNestedMap<Key, Value, Compare, SizeType, Mem, CheckFunc>::layout() const {
    std::stringstream ss;
    ss << "SharedNestedMap this=[" << (void*)this << "]";
    if (this->_info->_meta != nullptr) {
        ss << std::endl << *this->_info->_meta;
    }
    if (this->_info->_header != nullptr) {
        ss << std::endl << *this->_info->_header;
    }
    if (this->_object != nullptr) {
        ss << std::endl << container_layout(this->_object);
    }
    return ss.str();
}

template <class Key, class Value, class Compare, class SizeType>
inline size_t container_memsize(const CustomVector<Map<Key, Value, Compare, SizeType> > *object) {
    return (object->empty() ? sizeof(*object) : (size_t)object->back().cend() - (size_t)object);
};

template <class Key, class Value, class Compare, class SizeType>
inline std::string container_layout(const CustomVector<Map<Key, Value, Compare, SizeType> > *object) {
    std::stringstream ss;
    ss << "CustomVector Nested Map this=[" << (void*)object << "]"
       << nested_container_layout<Map<Key, Value, Compare, SizeType> >(object);
    return ss.str();
}

}  // namespace levin

#endif  // LEVIN_SHARED_VECTOR_NESTED_MAP_HPP
