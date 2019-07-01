#ifndef LEVIN_SHARED_HASHSET_H
#define LEVIN_SHARED_HASHSET_H

#include <unordered_set>
#include <vector>
#include "details/hashset.hpp"
#include "svec.hpp"

namespace levin {

template <class Key,
          class Hash = std::hash<Key>,
          class Pred = std::equal_to<Key>,
          class CheckFunc = levin::IntegrityChecker>
class SharedHashSet : public SharedBase {
public:
    typedef HashSet<Key, Hash, Pred>                 container_type;
    typedef typename container_type::impl_type       impl_type;
    typedef typename container_type::bucket_type     bucket_type;
    typedef typename container_type::value_type      value_type;
    typedef typename container_type::value_size_type value_size_type;
    typedef typename container_type::size_type       size_type;

    SharedHashSet() : SharedBase(), _object(nullptr) {
    }
    SharedHashSet(const std::string &name, const std::string group = "default", const int id = 1) :
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
    template <class Alloc>
    static bool Dump(
            const std::string &file,
            const std::unordered_set<Key, Hash, Pred, Alloc> &hashset);

    bool empty() const { return _object->empty(); }
    size_t size() const { return _object->size(); }
    size_t bucket_count() const { return _object->bucket_count(); }
    // @brief no range check, as std unordered_set
    size_t bucket_size(size_t n) const { return _object->bucket_size(n); }
    const Key* cbegin() const { return _object->cbegin(); }
    const Key* cend() const { return _object->cend(); }
    // ret type const refrence for reminder readonly
    const Key* begin() const { return _object->begin(); }
    const Key* end() const { return _object->end(); }
    const Key* find(const Key& key) const { return _object->find(key); }
    size_t count(const Key& key) const { return _object->count(key); }

    std::string layout() const;

protected:
    container_type *_object = nullptr;
};

template <class Key, class Hash, class Pred, class CheckFunc>
bool SharedHashSet<Key, Hash, Pred, CheckFunc>::Dump(const std::string &file) {
    return this->_bin2file(file, container_memsize(_object), _object);
}

template <class Key, class Hash, class Pred, class CheckFunc>
template <class Alloc>
bool SharedHashSet<Key, Hash, Pred, CheckFunc>::Dump(
        const std::string &file,
        const std::unordered_set<Key, Hash, Pred, Alloc> &hashset) {
    size_t bucket_count = hashset.bucket_count();
    LEVIN_CDEBUG_LOG("Dump(). file=%s, hashset size=%ld, bcount=%ld, bucket count=%ld",
            file.c_str(), hashset.size(), hashset.bucket_count(), bucket_count);
    std::vector<std::vector<Key> > datas(bucket_count, std::vector<Key>());
    Hash hasher;
    for (const auto &key : hashset) {
        datas[hasher(key) % bucket_count].emplace_back(key);
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

    std::vector<size_type> hashset_headers = {hashset.size(), bucket_count};
    fout.write((const char*)hashset_headers.data(), sizeof(size_type) * hashset_headers.size());
    CHECK_FILE_READ_OR_WRITE_RES(fout, file);

    return SharedNestedVector<value_type, value_size_type>::dump(file, datas, fout);
}

template <class Key, class Hash, class Pred, class CheckFunc>
std::string SharedHashSet<Key, Hash, Pred, CheckFunc>::layout() const {
    std::stringstream ss;
    ss << "SharedHashSet this=[" << (void*)this << "]";
    if (_info->_meta != nullptr) {
        ss << std::endl << *_info->_meta;
    }
    if (_object != nullptr) {
        ss << std::endl << _object->layout();
    }
    return ss.str();
}

}  // namespace levin

#endif  // LEVIN_SHARED_HASHSET_H
