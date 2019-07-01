#ifndef LEVIN_SHARED_BASE_H
#define LEVIN_SHARED_BASE_H

#include <boost/scoped_ptr.hpp>
#include "shared_utils.h"
#include "shared_allocator.h"
#include "xsi_shm.hpp"

namespace levin {

struct ContainerInfo {
    std::string _name;
    std::string _group;
    int _appid;
    CheckFunctor _checkfunc;
    boost::scoped_ptr<levin::SharedMemory> _shm;
    boost::scoped_ptr<levin::SharedAllocator> _alloc;
    SharedMeta *_meta;  // which located at shm region, use raw pointer
    bool _is_exist = false;

    ContainerInfo() : _name(""), _appid(0), _meta(nullptr), _is_exist(false) {
    }
    ContainerInfo(const std::string &name, const std::string &group, const int id, CheckFunctor check) :
            _name(name), _group(group), _appid(id), _checkfunc(check), _meta(nullptr), _is_exist(false) {
    }
};

// @brief base class of shared container
class SharedBase : public boost::noncopyable {
public:
    // @brief shared container version for workaround binary-incompatible implement upgrade
    // why not treated version specialized per container type
    // cause there are dependency relationship between them
    static const uint8_t SC_VERSION = 2;

    SharedBase() : _info(nullptr) {
    }
    SharedBase(
            const std::string &name,
            const std::string &group,
            const int appid = 1,
            CheckFunctor check = levin::IntegrityChecker()) :
            _info(new ContainerInfo(name, group, appid, check)) {
    }
    virtual ~SharedBase() {
    }

    virtual int Init() = 0;
    virtual int Load() = 0;
    virtual bool Dump(const std::string &file) = 0;

    bool IsExist() const {
        return _info->_is_exist;
    }

    void Destroy() {
        _info->_meta = nullptr;
        _info->_shm->remove_shared_memory();
    }

protected:
    template <typename Container>
    int _init(Container *&ptr);

    template <typename Container>
    bool _check(const Container *ptr, bool is_upd);

    template <typename Container>
    int _load(Container *&ptr);

    template <typename Container>
    bool _file2bin(const std::string &file, Container *&ptr);

    template <typename Container>
    bool _bin2file(const std::string &file, const size_t container_size, const Container *ptr);

protected:
    boost::scoped_ptr<ContainerInfo> _info;
};

template <typename Container>
int SharedBase::_init(Container *&ptr) {
    _info->_shm.reset(new levin::SharedMemory(_info->_name, _info->_appid));
    if (_info->_shm.get() == nullptr) {
        LEVIN_CWARNING_LOG("new SharedMemory failed. name=%s", _info->_name.c_str());
        return SC_RET_OOM;
    }
    int ret = _info->_shm->init(SharedAllocator::Allocsize(sizeof(SharedMeta)));
    if (ret != SC_RET_OK) {
        return ret;
    }
    _info->_alloc.reset(
            new levin::SharedAllocator(_info->_shm->get_address(), _info->_shm->get_size()));
    if (_info->_alloc.get() == nullptr) {
        LEVIN_CWARNING_LOG("new SharedAllocator failed. name=%s", _info->_name.c_str());
        return SC_RET_OOM;
    }
    try {
        if (_info->_shm->is_exist()) {
            _info->_meta = _info->_alloc->template Address<SharedMeta>();
            ptr = _info->_alloc->template Address<Container>();
            if (_check(ptr)) {
                _info->_is_exist = true;
                LEVIN_CINFO_LOG("shm already exist. name=%s, shmid=%d, bytes=%ld",
                        _info->_name.c_str(), _info->_shm->get_shmid(), _info->_shm->get_size());
                return SC_RET_OK;
            }
            LEVIN_CWARNING_LOG("shm already exist but check fail. name=%s\n%s",
                    _info->_name.c_str(), _info->_meta->layout().c_str());
        }
        // shm no exist create shm, construct and load binfile
        // shm exist but checked fail, reconstruct and reload
        _info->_alloc->Reset();
        _info->_meta = _info->_alloc->template Construct<SharedMeta>(
                _info->_name.c_str(), typeid(Container).name(), _info->_group.c_str(), _info->_appid,
                typeid(Container).hash_code(), makeFlags(SC_VERSION));
        ptr = _info->_alloc->template Construct<Container>();
    } catch (std::exception &e) {
        LEVIN_CWARNING_LOG("alloc failed, remove shm. what=%s, name=%s",
                e.what(), _info->_name.c_str());
        ptr = nullptr;
        Destroy();
        return SC_RET_ALLOC_FAIL;
    }
    LEVIN_CDEBUG_LOG("construct meta&ptr mem used=%ld, _meta=%p, ptr=%p",
            _info->_alloc->used_size(), (void*)_info->_meta, ptr);
    return SC_RET_OK;
}

template <typename Container>
bool SharedBase::_check(const Container *ptr, bool is_upd = false) {
    if (typeid(Container).hash_code() != _info->_meta->hashcode) {
        LEVIN_CWARNING_LOG("checked summary hash failed. %s NOT matches %s",
                demangle(typeid(Container).name()).c_str(), _info->_meta->summary);
        return false;
    }
    if (SC_VERSION != VersionOfFlags(_info->_meta->flags)) {
        LEVIN_CWARNING_LOG("Shared container version %u NOT matches %ld",
                SC_VERSION, _info->_meta->flags);
        return false;
    }

    SharedChecksumInfo info((void*)ptr, container_memsize(ptr));
    size_t fixed_len = SharedAllocator::Allocsize(sizeof(SharedMeta));
    if (_info->_alloc->OutOfRange(
                (void*)((size_t)info.area - fixed_len), info.length + fixed_len)) {
        LEVIN_CWARNING_LOG("checked region out of range.");
        return false;
    }
    return _info->_checkfunc(info, _info->_meta, is_upd);
}

template <typename Container>
int SharedBase::_load(Container *&ptr) {
    if (_info->_is_exist) {
        LEVIN_CDEBUG_LOG("no need load, use exist shm directly. name=%s", _info->_name.c_str());
        return SC_RET_OK;
    }
    if (ptr == nullptr || !_file2bin(_info->_name, ptr)) {
        LEVIN_CWARNING_LOG("load failed, remove shm. name=%s", _info->_name.c_str());
        ptr = nullptr;
        Destroy();
        return SC_RET_LOAD_FAIL;
    }
    // double check: this container out of shm range; overlapped by other container
    if (!_check(ptr, true)) {
        LEVIN_CWARNING_LOG("load end but checked fail, remove shm. name=%s\n%s",
                _info->_name.c_str(), _info->_meta->layout().c_str());
        ptr = nullptr;
        Destroy();
        return SC_RET_CHECK_FAIL;
    }
    LEVIN_CINFO_LOG("file load succ. name=%s, shmid=%d, bytes=%ld",
            _info->_name.c_str(), _info->_shm->get_shmid(), _info->_shm->get_size());
    return SC_RET_OK;
}

template <typename Container>
bool SharedBase::_file2bin(const std::string &file, Container *&ptr) {
    std::ifstream fin(file, std::ios::in | std::ios::binary);
    if (!fin.is_open()) {
        LEVIN_CWARNING_LOG("open file for read fail. file=%s", file.c_str());
        return false;
    }
    // read file header: used memory size/container type hashcode
    SharedFileHeader header;
    fin.read((char*)&header, sizeof(header));
    if (!fin) {
        LEVIN_CWARNING_LOG("read file fail. file=%s", file.c_str());
        return false;
    }
    // validate expected container type hashcode
    if (typeid(Container).hash_code() != header.type_hash) {
        LEVIN_CWARNING_LOG("validate typeid hash in file failed. file=%s, T=%s",
                file.c_str(), demangle(typeid(Container).name()).c_str());
        return false;
    }
    // validate expected container implement version
    //if (SC_VERSION != VersionOfFlags(header.flags)) {
    //    LEVIN_CWARNING_LOG("validate container version failed. file=%s", file.c_str());
    //    return false;
    //}

    // read container bin
    size_t container_size = header.container_size;
    fin.read((char*)ptr, container_size);
    if (!fin) {
        LEVIN_CWARNING_LOG("read file fail. file=%s", file.c_str());
        return false;
    }
    fin.close();
    LEVIN_CDEBUG_LOG("file2bin file=%s, T=%s, container size=%ld",
            file.c_str(), typeid(Container).name(), container_size);
    return true;
}

template <typename Container>
bool SharedBase::_bin2file(
        const std::string &file, const size_t container_size, const Container *ptr) {
    std::ofstream fout(file, std::ios::out | std::ios::binary);
    if (!fout.is_open()) {
        LEVIN_CWARNING_LOG("open file for write fail. file=%s", file.c_str());
        return false;
    }

    // write file header: used memory size/container type hash
    size_t type_hash = typeid(Container).hash_code();
    SharedFileHeader header = {container_size, type_hash, makeFlags(SC_VERSION)};
    fout.write((const char*)&header, sizeof(header));
    if (!fout) {
        LEVIN_CWARNING_LOG("write file fail. file=%s", file.c_str());
        return false;
    }
    // write capacity/size/offset/array
    fout.write((const char*)ptr, container_size);
    if (!fout) {
        LEVIN_CWARNING_LOG("write file fail. file=%s", file.c_str());
        return false;
    }
    fout.close();
    LEVIN_CDEBUG_LOG("bin2file file=%s, T=%s, container size=%ld",
            file.c_str(), typeid(Container).name(), container_size);
    return true;
}

}  // namespace levin

#endif  // LEVIN_SHARED_BASE_H
