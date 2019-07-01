#ifndef LEVIN_SHARED_HASHMAP_H
#define LEVIN_SHARED_HASHMAP_H

#include <unordered_map>
#include "details/hashmap.hpp"
#include "shared_base.hpp"
#include "svec.hpp"

namespace levin {

template<class Key, class Value>
class HashIterator;

template <class Key,
          class Value,
          class Hash = std::hash<Key>,
          class CheckFunc = levin::IntegrityChecker>
class SharedHashMap : public SharedBase {
public:
    typedef HashMap<Key, Value, Hash>                container_type;
    typedef typename container_type::impl_type       impl_type;
    typedef typename container_type::bucket_type     bucket_type;
    typedef typename container_type::value_type      value_type;
    typedef typename container_type::value_size_type value_size_type;
    typedef typename container_type::size_type       size_type;
    typedef HashIterator<Key, Value> iterator;
    typedef HashIterator<Key, Value> const_iterator;
    friend class HashIterator<Key, Value>;
public:
    SharedHashMap(const std::string &name, const std::string group = "default", const int id = 1) :
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
    iterator find(Key key) { return _object->find(key); }
    const_iterator find(Key key) const { return _object->find(key); }
    
    iterator begin() { return _object->begin(); }
    const_iterator begin() const { return _object->begin(); }
    iterator end() { return _object->end(); }
    const_iterator end() const { return _object->end(); }

    size_t count(const Key& key) { return _object->count(key); }
    // @brief mybe THROW
    // STD unordered_map performing an insertion if such key does not already exist
    // Levin hashmap will throw out_of_range if key does not exist, just as at
    const Value& operator[](const Key& key) { return find(key)->second; }
    const Value& at(const Key& key) const { return _object->at(key); }

    std::string layout() const {
        std::stringstream ss;
        ss << "SharedHashMap this=[" << (void*)this << "]";
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
            return (size_t)_object->datas().back().cend() - (size_t)_info->_meta;
        }
        return 0;
    }
private:
    container_type *_object = nullptr;
};

template <class Key, class Value, class Hash, class CheckFunc>
bool SharedHashMap<Key, Value, Hash, CheckFunc>::Dump(const std::string &file) {
    return _bin2file(file, container_memsize(this->_object), _object);
}

template <class Key, class Value, class Hash, class CheckFunc>
template <class T>
bool SharedHashMap<Key, Value, Hash, CheckFunc>::Dump(const std::string &file, const T &map) {
    if (map.empty()) {
        LEVIN_CWARNING_LOG("input container is empty.");
        return false;
    }
    // key: hash buckets idx
    // val: [ pair<key, value> ]
    size_t bucket_size = getPrime(map.size());
    LEVIN_CDEBUG_LOG("Dump(). file=%s, map size=%ld, bucket size=%ld",
            file.c_str(), map.size(), bucket_size);
    Hash hashfun;
    std::vector<std::vector<std::pair<Key, Value> > > datas(
            bucket_size, std::vector<std::pair<Key, Value> >());
    for (auto it = map.begin(); it != map.end(); ++it) {
        datas[hashfun(it->first) % bucket_size].emplace_back(std::make_pair(it->first, it->second));
    }
    for (auto &row : datas) {
        std::sort(row.begin(), row.end(), CMP<Key, Value>);
    }
    std::ofstream fout(file, std::ios::out | std::ios::binary);
    if (!fout.is_open()) {
        LEVIN_CWARNING_LOG("open file for write fail. file=%s", file.c_str());
        return false;
    }
    // write file header: used memory size(meta size + container size)/container type hash
    size_t container_size = sizeof(container_type);
    for (auto &row : datas) {
        container_size += sizeof(bucket_type);
        container_size += row.size() * sizeof(value_type);
    }
    size_t type_hash = typeid(container_type).hash_code();
    SharedFileHeader header = {container_size, type_hash, makeFlags(SharedBase::SC_VERSION)};
    fout.write((const char*)&header, sizeof(header));
    CHECK_FILE_READ_OR_WRITE_RES(fout, file);

    std::vector<size_type> imap_headers = {map.size(), bucket_size};
    fout.write((const char*)imap_headers.data(), sizeof(size_type) * imap_headers.size());
    CHECK_FILE_READ_OR_WRITE_RES(fout, file);

    bool ret = SharedNestedVector<value_type, value_size_type>::dump(file, datas, fout);
    fout.close();

    return ret;
}

template <class Key, class Value>
inline std::ostream& operator<<(std::ostream &os, const SharedHashMap<Key, Value> &map) {
    return os << map.layout();
}
 
}  // namespace levin

#endif  // LEVIN_SHARED_HASHMAP_H
