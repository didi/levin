#ifndef LEVIN_XSI_SHM_H
#define LEVIN_XSI_SHM_H

#include <fstream>
#include <boost/interprocess/xsi_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/algorithm/string.hpp>
#include "shared_utils.h"

namespace levin {

namespace bip = boost::interprocess;

const size_t MAX_MEM_SIZE = 60000000000; // Maximum limit 60G
const std::string LEVIN("levin");

class SharedMemory {
public:
    SharedMemory(const std::string &path, int id = 1, const size_t mem_size = 0) :
        _path(path),
        _mem_size(mem_size),
        _id(id) {
    }
    virtual ~SharedMemory() {
        SAFE_DELETE(_region_ptr);

        SAFE_DELETE(_shm_ptr);
    }
    bool remove_shared_memory() {
        try {
            if (_region_ptr && _shm_ptr) {
                SAFE_DELETE(_region_ptr);
                bip::xsi_shared_memory::remove(_shm_ptr->get_shmid());
                SAFE_DELETE(_shm_ptr);
            }
        }
        catch(bip::interprocess_exception &e) {
            if (e.get_error_code() != bip::not_found_error) {
                LEVIN_CWARNING_LOG("remove shm failed, shmid: %d", _shm_ptr->get_shmid());
                return false;
            }
        }
        return true;
    }
    static bool remove_shared_memory(int shmid) {
        try {
            bip::xsi_shared_memory::remove(shmid);
        }
        catch (bip::interprocess_exception &e) {
            if (e.get_error_code() != bip::not_found_error) {
                LEVIN_CWARNING_LOG("remove shm failed, shmid: %d", shmid);
                return false;
            }
        }
        return true;
    }
    //@brief get shmids from the system with identifier 'levin'
    //"no_attach" is for Whether to get no application attached shared memory
    //return file->shmid
    static bool get_all_shmid(std::vector<SharedMidInfo> &shmid_info, bool no_attach = true) {
        std::vector<int> vec_shmid;
        struct shmid_ds shm_info;
        struct shmid_ds shm_segment;
        int max_id = ::shmctl(0, SHM_INFO, &shm_info);
        for (int i=0; i<=max_id; ++i) {
            int shm_id = ::shmctl(i, SHM_STAT, &shm_segment);
            if (shm_id <= 0)
                continue;
            else {
                if (no_attach) {
                    if (shm_segment.shm_nattch == 0) {
                        vec_shmid.emplace_back(shm_id);
                    }
                }
                else {
                    vec_shmid.emplace_back(shm_id);
                }
            }
        }
        shmid_info.clear();
        for (auto mid: vec_shmid) {
            try {
                bip::xsi_shared_memory shm(bip::open_only, mid);
                bip::mapped_region  region(shm, bip::read_write);
                SharedMeta *meta = static_cast<SharedMeta *>(region.get_address());
                if (std::string(meta->summary).find(LEVIN) != std::string::npos) {
                    shmid_info.emplace_back(SharedMidInfo(meta->path, mid, meta->group, meta->id));
                } else {
                    LEVIN_CWARNING_LOG("maybe no levin create, shmid:%d",mid);
                }
            }
            catch (bip::interprocess_exception &ex) {
                LEVIN_CWARNING_LOG("get shm failed, code:%d, info:%s, mid:%d", ex.get_error_code(), ex.what(), mid);
            }
            catch (...) {
                LEVIN_CWARNING_LOG("unknown_exception, shmid:%d", mid);
            }
        }
        return true;
    }
    //@brief Returns the shared memory ID that identifies the shared memory
    inline int get_shmid() const {
        return  _shm_ptr == nullptr ? 0:_shm_ptr->get_shmid();
    }

    //@brief Returns the base address of the shared memory
    inline void* get_address() const {
        return  _region_ptr == nullptr ? nullptr:_region_ptr->get_address();
    }
    inline bool is_exist() const {
        return _is_exist;
    }
    inline std::size_t get_size() const {
        return _region_ptr == nullptr ? 0:_region_ptr->get_size();
    }
    //@brief Open or Create System V shared memory
    //Returns false on error.
    inline uint32_t init(const size_t fixed_size = 0) {
        std::ifstream fin(_path, std::ios::in | std::ios::binary);
        if (!fin.is_open()) {
            return SC_RET_FILE_NOEXIST;
        }
        int proj_id = make_proj_id(_path, _id);
        bip::xsi_key key(_path.c_str(), proj_id);
        if (_mem_size == 0) {
            fin.read((char*)&_mem_size, sizeof(_mem_size));
            if (!fin) {
                fin.close();
                return SC_RET_READ_FAIL;
            }
        }
        _mem_size += fixed_size;
        if (_mem_size == 0 || _mem_size >= MAX_MEM_SIZE) {
            LEVIN_CWARNING_LOG("apply shm fail, mem size: %lu", _mem_size);
            fin.close();
            return SC_RET_SHM_SIZE_ERR;
        }
        try {
            _shm_ptr = new bip::xsi_shared_memory(bip::create_only, key, _mem_size);
        } catch (bip::interprocess_exception &ex) {
            if (ex.get_error_code() == bip::already_exists_error) {
                _shm_ptr = new bip::xsi_shared_memory(bip::open_only, key);
                _is_exist = true;
            }
        }
        fin.close();
        if (nullptr == _shm_ptr) {
            return SC_RET_OOM;
        }
        _region_ptr = new bip::mapped_region(*_shm_ptr, bip::read_write);
        if (nullptr == _region_ptr) {
            return SC_RET_OOM;
        }

        LEVIN_CINFO_LOG("shm init succ. path=%s, shmid=%d, size=%ld, region=[%p,%p)",
                _path.c_str(), get_shmid(), get_size(), get_address(),
                (void*)((size_t)get_address() + get_size()));

        return SC_RET_OK;
    }
private:
    int make_proj_id(std::string path, int id){
        std::hash<std::string> hashFunc;
        size_t hash_id = hashFunc(path);
        uint8_t proj_id = 0;
        const char *p = reinterpret_cast<const char*>(&hash_id);
        for (uint32_t i = 0; i<sizeof(hash_id); i++) {
            proj_id ^= p[i];
        }
        return proj_id ^ id;
    }

protected:
    std::string _path;
    size_t _mem_size;
    int _id;
    bip::xsi_shared_memory* _shm_ptr = nullptr;
    bip::mapped_region* _region_ptr = nullptr;
    bool _is_exist = false;
};

} //endif levin

#endif  // LEVIN_XSI_SHM_H
