#include "id_manager.h"
#include <iostream>
#include "xsi_shm.hpp"

namespace levin {

IdManager::ObjectCreator IdManager::creator;

bool IdManager::init() {
    std::vector<SharedMidInfo> infos;
    if (!get_all_shmid(infos, false)) {
        return false;
    }
    for (const auto &info : infos) {
        _id_map.emplace(std::make_pair(info.mid, info.path));
        _shm_map.emplace(std::make_pair(info.path, info.mid));
    }
    for (const auto &pair : _shm_map) {
        std::cout << "init name2id. " << pair.first << "," << pair.second << std::endl;
    }
    std::cout << "IdManager init done. " << std::endl;
    return true;
}

bool IdManager::Register(const int id, const std::string &name) {
    if (!insertWithLock(id, name)) {
        LEVIN_CWARNING_LOG("Duplicated share memory in IdManager. id=%d, name=%s", id, name.c_str());
        return false;
    }
    LEVIN_CDEBUG_LOG("Register. id=%d, name=%s", id, name.c_str());
    return true;
}

bool IdManager::DeRegister(const int id) {
    if (!deleteWithLock(id)) {
        LEVIN_CWARNING_LOG("Not find share memory in IdManager. id=%d", id);
        return false;
    }
    LEVIN_CDEBUG_LOG("DeRegister. id=%d", id);
    return true;
}

bool IdManager::insertWithLock(const int id, const std::string &name) {
    std::lock_guard<std::mutex> guard(_mutex);
    if (_id_map.find(id) != _id_map.end() || _shm_map.find(name) != _shm_map.end()) {
        return false;
    }
    _id_map.emplace(std::make_pair(id, name)); 
    _shm_map.emplace(std::make_pair(name, id));
    return true;
}

bool IdManager::deleteWithLock(const int id) {
    std::lock_guard<std::mutex> guard(_mutex);
    auto ret = _id_map.find(id);
    if (ret == _id_map.end()) {
        return false;
    }
    std::string name = ret->second;
    _id_map.erase(id);
    _shm_map.erase(name);
    return true;
}

bool IdManager::GetId(const std::string &name, int &id) const {
    std::lock_guard<std::mutex> guard(_mutex);
    auto ret = _shm_map.find(name);
    if (ret == _shm_map.end()) {
        return false;
    }
    id = ret->second;
    return true;
}

bool IdManager::get_all_shmid(std::vector<SharedMidInfo> &shmid_info, bool no_attach) {
    static const std::string LEVIN_PATTERN = "levin";
    struct shmid_ds shm_info;
    int max_id = ::shmctl(0, SHM_INFO, &shm_info);
    std::vector<int> shmid_vec;
    struct shmid_ds shm_segment;
    for (int id = 0; id <= max_id; ++id) {
        int shm_id = ::shmctl(id, SHM_STAT, &shm_segment);
        if (shm_id <= 0) {
            std::cout << "stat shm failed, ignore. id=" << id << std::endl;
            continue;
        }
        if (!no_attach || shm_segment.shm_nattch == 0) {
            shmid_vec.emplace_back(shm_id);
        }
    }
    shmid_info.clear();
    for (auto shmid: shmid_vec) {
        MappedRegion mapped_region(shmid);
        int ret = mapped_region.attach();
        if (ret != 0) {
            std::cout << "attach shm failed, shmid:" << shmid << ", errno=" << ret << std::endl;
            continue;
        }
        SharedMeta *meta = static_cast<SharedMeta*>(mapped_region.get_address());
        if (std::string(meta->summary).find(LEVIN_PATTERN) != std::string::npos) {
            shmid_info.emplace_back(meta->path, shmid, meta->group, meta->id);
        } else {
            std::cout << "unknown shm, not created by levin, shmid:" << shmid << std::endl;
        }
    }
    return true;
}

}  // namespace levin
