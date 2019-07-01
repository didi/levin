#ifndef LEVIN_SHARED_NESTED_HASHMAP_H
#define LEVIN_SHARED_NESTED_HASHMAP_H

#include <vector>
#include <unordered_map>
#include "shared_base.hpp"
#include "details/nested_hashmap.hpp"
#include "svec.hpp"

namespace levin {

template <class Key,
          class Value,
          class Hash = std::hash<Key>,
          class CheckFunc = levin::IntegrityChecker>
class SharedNestedHashMap : public SharedBase {
public:
    typedef NestedHashMap<Key, Value, Hash>                container_type;
    typedef typename container_type::index_impl_type       index_impl_type;
    typedef typename container_type::data_impl_type        data_impl_type;
    typedef typename container_type::index_bucket_type     index_bucket_type;
    typedef typename container_type::data_array_type       data_array_type;
    typedef typename container_type::index_value_type      index_value_type;
    typedef typename container_type::index_value_size_type index_value_size_type;
    typedef typename container_type::data_value_type       data_value_type;
    typedef typename container_type::data_value_size_type  data_value_size_type;
    typedef typename container_type::size_type             size_type;
    typedef NHashIterator<Key, Value> iterator;
    typedef NHashIterator<Key, Value> const_iterator;
    friend class NHashIterator<Key, Value>;
public:
    SharedNestedHashMap(
            const std::string &name, const std::string group = "default", const int id = 1) :
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
    template <class T>
    static bool Dump(const std::string &file, const T &map);

    bool empty() const { return _object->size() == 0; }
    size_t size() const { return _object->size(); }
    size_t bucket_size() const { return _object->bucket_size(); }

    iterator begin() { return _object->begin(); }
    const_iterator begin() const { return _object->begin(); }
    iterator end() { return _object->end(); }
    const_iterator end() const  { return _object->end(); }

    iterator find(Key key) { return _object->find(key); }
    size_t count(const Key& key) { return _object->count(key); }
    const data_array_type* operator[](const Key& key) { return find(key)->second; }

    std::string layout() const {
        std::stringstream ss;
        ss << "SharedNestedHashMap this=[" << (void*)this << "]";
        if (this->_info->_meta != nullptr) {
             ss << std::endl << *_info->_meta;
        }
        if (this->_object != nullptr) {
             ss << std::endl << this->_object->layout();
        }
        return ss.str();
    }
    //for ut/debug
    size_t load_shm_size() const {
        if (_info && _object && _object->data_array() && !_object->data_array()->empty()) {
            return (size_t)_object->data_array()->back().cend() - (size_t)_info->_meta;
        }
        return 0;
    }

private:
    container_type *_object = nullptr;
};

template <class Key, class Value, class Hash, class CheckFunc>
bool SharedNestedHashMap<Key, Value, Hash, CheckFunc>::Dump(const std::string &file) {
    return _bin2file(file, container_memsize(this->_object), _object);
}

template <class Key, class Value, class Hash, class CheckFunc>
template <class T>
bool SharedNestedHashMap<Key, Value, Hash, CheckFunc>::Dump(const std::string &file, const T &map) {
    std::ofstream fout(file, std::ios::out | std::ios::binary);
    if (!fout.is_open()) {
        LEVIN_CWARNING_LOG("open file for write fail. file=%s", file.c_str());
        return false;
    }
    if (map.empty()) {
        LEVIN_CWARNING_LOG("input container is empty.");
        return false;
    }
    // key: hash buckets idx
    // index: [ pair<key, pos> ]
    // data:  [pos ->CustomVector<Value>]
    size_t bucket_size = getPrime(map.size());
    LEVIN_CDEBUG_LOG("Dump(). file=%s, map size=%ld, bucket size=%ld", file.c_str(), map.size(), bucket_size);
    std::vector<std::vector<std::pair<Key, size_t> > > idx_datas(bucket_size, std::vector<std::pair<Key, size_t> >());
    std::vector<std::vector<Value> > value_datas(map.size(), std::vector<Value>());
    Hash hashfun;
    size_t pos = 0;
    for (auto it = map.begin(); it != map.end(); ++it, ++pos) {
        idx_datas[hashfun(it->first) % bucket_size].emplace_back(std::make_pair(it->first, pos));
        value_datas[pos].assign(it->second.begin(), it->second.end());
    }
    for (auto &row : idx_datas) {
        std::sort(row.begin(), row.end(), CMP<Key, size_t>);
    }

    // sizeof NestedHashMap contains sizeof index data
    size_t container_size = sizeof(container_type);
    size_t index_size = 0;
    for (auto &row : idx_datas) {
        index_size += sizeof(index_bucket_type);
        index_size += row.size() * sizeof(index_value_type);
    }
    container_size += index_size;

    // data array size
    size_t data_size = sizeof(data_impl_type);
    for (auto &row : value_datas) {
        data_size += sizeof(data_array_type);
        data_size += row.size() * sizeof(data_value_type);
    }
    container_size += data_size;

    // write file header: used memory size(meta size + container size)/container type hash
    size_t type_hash = typeid(container_type).hash_code();
    SharedFileHeader header = {container_size, type_hash, makeFlags(SharedBase::SC_VERSION)};
    fout.write((const char*)&header, sizeof(header));
    CHECK_FILE_READ_OR_WRITE_RES(fout, file);

    std::vector<size_type> imap_headers = {map.size(), bucket_size, index_size, data_size};
    fout.write((const char*)imap_headers.data(), sizeof(size_type) * imap_headers.size());
    CHECK_FILE_READ_OR_WRITE_RES(fout, file);

    bool ret = SharedNestedVector<index_value_type, index_value_size_type>::dump(file, idx_datas, fout) &&
               SharedNestedVector<data_value_type, data_value_size_type>::dump(file, value_datas, fout);
    fout.close();

    return ret;
}

template <class Key, class Value, class Hash>
inline std::ostream& operator<<(
        std::ostream &os, const SharedNestedHashMap<Key, Value, Hash> &map) {
    return os << map.layout();
}
    
}  // namespace levin

#endif  // LEVIN_SHARED_NESTED_HASHMAP_H
