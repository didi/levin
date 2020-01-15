#ifndef LEVIN_SHARED_MEMORY_HPP
#define LEVIN_SHARED_MEMORY_HPP

#include <fstream>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include "xsi_shm.hpp"
#include "shared_utils.h"
#include "id_manager.h"

namespace levin {

const size_t MAX_MEM_SIZE = 60000000000; // Maximum limit 60G
const std::string LEVIN_PATTERN("levin");

// @brief memory base class
class MemoryBase : public boost::noncopyable {
public:
    MemoryBase(const std::string &path, int id = 1, const size_t mem_size = 0) :
        _path(path),
        _mem_size(mem_size),
        _id(id) {
    }
    virtual ~MemoryBase() {
    }

    // @brief Open or create memory
    virtual int init(const size_t fixed_size) {
        std::ifstream fin(_path, std::ios::in | std::ios::binary);
        if (!fin.is_open()) {
            return SC_RET_FILE_NOEXIST;
        }
        if (_mem_size == 0) {
            fin.read((char*)&_mem_size, sizeof(_mem_size));
            if (!fin) {
                return SC_RET_READ_FAIL;
            }
        }
        _mem_size += fixed_size;
        if (_mem_size == 0 || _mem_size >= MAX_MEM_SIZE) {
            LEVIN_CWARNING_LOG("init memory fail. illegal size=%lu", _mem_size);
            return SC_RET_SHM_SIZE_ERR;
        }
        return SC_RET_OK;
    }
    virtual bool remove() = 0;

    virtual bool is_exist() const { return false; }
    virtual void* get_address() const { return nullptr; }
    virtual std::size_t get_size() const { return _mem_size; }
    virtual const std::string &info() const { return _info; }

protected:
    std::string _path;
    size_t _mem_size = 0;
    int _id = 0;
    std::string _info;
};

// @brief Heap memory
class HeapMemory : public MemoryBase {
public:
    HeapMemory(const std::string &path, int id = 1, const size_t mem_size = 0) :
        MemoryBase(path, id, mem_size) {
    }
    virtual ~HeapMemory() {
        remove();
    }

    virtual int init(const size_t fixed_size) override {
        int ret = MemoryBase::init(fixed_size);
        if (ret != SC_RET_OK) {
            return ret;
        }

        _ptr = (void*)malloc(_mem_size);
        if (_ptr == nullptr) {
            return SC_RET_OOM;
        }
        set_info();
        LEVIN_CINFO_LOG("heap memory init succ. path=%s, info=%s", _path.c_str(), _info.c_str());
        return SC_RET_OK;
    }
    virtual bool remove() override {
        if (_ptr != nullptr) {
            free(_ptr);
            _ptr = nullptr;
        }
        return true;
    }

    virtual void* get_address() const override { return _ptr; }

private:
    void set_info() {
        std::stringstream ss;
        ss << "HeapMemory size=" << get_size()
           << " region=["
           << get_address() << "," << (void*)((size_t)get_address() + get_size()) << ")";
        _info = ss.str();
    }

private:
    void* _ptr = nullptr;
};

// @brief System V share memory
class SharedMemory : public MemoryBase {
public:
    SharedMemory(const std::string &path, int id = 1, const size_t mem_size = 0) :
        MemoryBase(path, id, mem_size) {
    }
    virtual ~SharedMemory() {
    }

    // @brief Open or create shared memory
    virtual int init(const size_t fixed_size) override;
    virtual bool remove() override;

    //@brief Return base address of share memory
    virtual void* get_address() const override {
        return (_region_ptr.get() == nullptr ? nullptr : _region_ptr->get_address());
    }
    virtual std::size_t get_size() const override {
        return (_region_ptr.get() == nullptr ? 0 : _region_ptr->get_size());
    }
    virtual bool is_exist() const override {
        return _is_exist;
    }

    //@brief Return ID which identifies the share memory
    int get_shmid() const {
        return _shmid;
    }

    static bool remove_shared_memory(int shmid);
    // @brief get shmids from the system with identifier 'levin'
    // "no_attach" is for Whether to get no application attached shared memory
    // return file->shmid
    static bool get_all_shmid(std::vector<SharedMidInfo> &shmid_info, bool no_attach = true);

private:
    bool check_path();
    void set_info() {
        std::stringstream ss;
        ss << "SharedMemory shmid=" << get_shmid()
           << " size=" << get_size()
           << " region=["
           << get_address() << "," << (void*)((size_t)get_address() + get_size()) << ")";
        _info = ss.str();
    }

private:
//    std::string _path;
//    size_t _mem_size = 0;
//    int _id = 0;
    int _shmid = IPC_PRIVATE;
    boost::scoped_ptr<MappedRegion> _region_ptr;
    bool _is_exist = false;
};

inline int SharedMemory::init(const size_t fixed_size) {
    int ret = MemoryBase::init(fixed_size);
    if (ret != SC_RET_OK) {
        return ret;
    }

    boost::scoped_ptr<XsiSharedMemory> shm_ptr(new XsiSharedMemory);
    if (shm_ptr.get() == nullptr) {
        return SC_RET_OOM;
    }
    if (IdManager::GetInstance().GetId(_path, _shmid)) {
        _is_exist = true;
    } else {
        if (shm_ptr->open(XsiShmCreateMode::Create, _shmid, _mem_size) != 0) {
            return SC_RET_ERR_SYS;
        }
        _shmid = shm_ptr->get_shmid();
        IdManager::GetInstance().Register(_shmid, _path);
    }
    LEVIN_CINFO_LOG("path=%s, shmid=%d, is_exist=%d", _path.c_str(), get_shmid(), is_exist());
    _region_ptr.reset(new MappedRegion(_shmid));
    if (_region_ptr.get() == nullptr) {
        return SC_RET_OOM;
    }
    ret = _region_ptr->attach();
    if (ret == ENOMEM) {
        return SC_RET_OOM;
    }
    if (ret != 0) {
        LEVIN_CWARNING_LOG("shared memory init fail. system errno=%d", ret);
        return SC_RET_ERR_SYS;
    }
    // double check path desc of the existed shm
    if (_is_exist && !check_path()) {
        return SC_RET_SHM_KEY_CONFLICT;
    }
    set_info();
    LEVIN_CINFO_LOG("shared memory init succ. path=%s, info=%s", _path.c_str(), _info.c_str());
    return SC_RET_OK;
}

inline bool SharedMemory::remove() {
    if (_region_ptr.get() != nullptr) {
        _region_ptr.reset();
    }
    if (!XsiSharedMemory::remove(_shmid)) {
        LEVIN_CWARNING_LOG("remove shm failed. path=%s, shmid=%d", _path.c_str(), _shmid);
        return false;
    }
    IdManager::GetInstance().DeRegister(_shmid);
    LEVIN_CINFO_LOG("remove shm succ. path=%s, shmid=%d", _path.c_str(), _shmid);
    return true;
}

inline bool SharedMemory::check_path() {
    if (_region_ptr.get() == nullptr) {
        return false;
    }
    SharedMeta *meta = static_cast<SharedMeta*>(_region_ptr->get_address());
    if (std::string(meta->summary).find(LEVIN_PATTERN) != std::string::npos &&
        _path.compare(meta->path) == 0) {
        return true;
    }
    LEVIN_CWARNING_LOG("shm key conflict, shm file:%s, load file:%s", meta->path, _path.c_str());
    return false;
}

inline bool SharedMemory::remove_shared_memory(int shmid) {
    if (!XsiSharedMemory::remove(shmid)) {
        LEVIN_CWARNING_LOG("remove shm failed, shmid: %d", shmid);
        return false;
    }
    IdManager::GetInstance().DeRegister(shmid);
    return true;
}

inline bool SharedMemory::get_all_shmid(std::vector<SharedMidInfo> &shmid_info, bool no_attach) {
    return IdManager::get_all_shmid(shmid_info, no_attach);
}

}  // namespace levin

#endif  // LEVIN_SHARED_MEMORY_HPP
