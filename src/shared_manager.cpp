
#include "shared_manager.h"
#include "boost/make_shared.hpp"
#include <boost/thread/thread.hpp>

namespace levin {

std::map<std::string, SharedContainerManager::ptr_status_pair> SharedContainerManager::_global_container_map;
std::map<std::string, SharedContainerManager::auth_func_pair> SharedContainerManager::_file_check_map;
std::set<std::string> SharedContainerManager::_has_checked_file_list;
bool SharedContainerManager::_clear_process_run = false;
boost::shared_ptr<boost::thread> SharedContainerManager::_clear_process = nullptr;
boost::shared_mutex SharedContainerManager::_wr_lock_global;
boost::shared_mutex SharedContainerManager::_wr_lock_container_init;


static SharedManagerGuard shared_manager_enter_exit_hook;

SharedManagerGuard::SharedManagerGuard() {
    SharedContainerManager::StartClearProcess();
}

SharedManagerGuard::~SharedManagerGuard() {
    SharedContainerManager::StopClearProcess();
}

SharedContainerManager::SharedContainerManager(const std::string group_name,  const int app_id) : 
        _group_name(group_name), _app_id(app_id) {}

SharedContainerManager::~SharedContainerManager() {
    boost_unique_lock lock(_wr_lock_local);
    for (auto ptr = _local_container_map.begin(); ptr != _local_container_map.end(); ptr++) {
        DeleteContainer(ptr->first);
    }
}

void SharedContainerManager::Release() {
    LEVIN_CINFO_LOG("Release function called, group id=[%s], container num=[%lu]", _group_name.c_str(), _local_container_map.size());
    boost_unique_lock lock(_wr_lock_local);
    for (auto ptr = _local_container_map.begin(); ptr != _local_container_map.end(); ptr++) {
        ReleaseContainer(ptr->first);
    }
    _local_container_map.clear();
}

int SharedContainerManager::GetAbsolutePath(const std::string &file_path, std::string &absolute_path) {
    if (file_path.empty()) {
        LEVIN_CWARNING_LOG("Get absolute path err, file path is empty");
        return SC_RET_FILE_NOEXIST;
    }
    if (file_path[0] == '/') {
        absolute_path = file_path;
        LEVIN_CDEBUG_LOG(
                "Get absolute path success, original path=[%s] absolute path=[%s]",
                file_path.c_str(),
                absolute_path.c_str());
        return SC_RET_OK;
    } else {
        char current_absolute_path[1024];
        if (NULL == realpath(file_path.c_str(), current_absolute_path)) {
            LEVIN_CWARNING_LOG(
                    "Get absolute path err, file_path=[%s], maybe file does't exist",
                    file_path.c_str());
            return SC_RET_FILE_NOEXIST;
        }
        absolute_path = current_absolute_path;
        LEVIN_CDEBUG_LOG(
                "Get absolute path success, original path=[%s] absolute path=[%s]",
                file_path.c_str(),
                absolute_path.c_str());
        return SC_RET_OK;
    }
}

int SharedContainerManager::AddLoading(const std::string &key_path, std::shared_ptr<SharedBase> shared_ptr) {
    {
        boost_unique_lock lock(_wr_lock_global);
        if (_global_container_map.find(key_path) != _global_container_map.end()) {
            LEVIN_CWARNING_LOG("container of file=[%s] has registed", key_path.c_str());
            return SC_RET_HAS_REGISTED;
        }
        _global_container_map[key_path] = std::make_pair(shared_ptr, STATUS_LOADING);
    }

    {
        boost_unique_lock lock(_wr_lock_local);
        _local_container_map[key_path] = shared_ptr;
    }
    return SC_RET_OK;
}

int SharedContainerManager::DeleteLoading(const std::string &key_path) {
    {
        boost_unique_lock lock(_wr_lock_global);
        if (_global_container_map.find(key_path) != _global_container_map.end()) {
            _global_container_map[key_path].first->Destroy();
            _global_container_map.erase(key_path);
        }
    }
    {
        boost_unique_lock lock(_wr_lock_local);
        _local_container_map.erase(key_path);
    }
    return SC_RET_OK;
}

int SharedContainerManager::UpdateSharedStatus(const std::string &key_path, const SharedContainerStatus status) {
    boost_unique_lock lock(_wr_lock_global);
    if (_global_container_map.find(key_path) == _global_container_map.end()) {
        LEVIN_CWARNING_LOG("container of file=[%s] is not exist", key_path.c_str());
        return SC_RET_NO_REGISTER;
    }
    _global_container_map[key_path].second = status;
    return SC_RET_OK;
}

void SharedContainerManager::DeleteContainer(const std::string &key_path) {
    boost_unique_lock lock(_wr_lock_global);
    if (_global_container_map.find(key_path) != _global_container_map.end()) {
        _global_container_map[key_path].second = STATUS_DELETING;
    }
}

void SharedContainerManager::ReleaseContainer(const std::string &key_path) {
    boost_unique_lock lock(_wr_lock_global);
    if (_global_container_map.find(key_path) != _global_container_map.end()) {
        _global_container_map[key_path].second = STATUS_RELEASING;
    }
}

void SharedContainerManager::VerifyFileProcess(std::map<std::string, std::string>& md5_file_map,
        boost::atomic<int>&  processed_file_idx,
        boost::atomic<int>&  left_thread_num,
        boost::atomic<bool>& is_check_stop,
        std::vector<bool>& check_md5_list_result,
        boost::mutex& cmutex,
        VerifyFileFuncPtr check_func) {
    while (true) {
        int file_idx = processed_file_idx++;
        int file_num = static_cast<int>(md5_file_map.size());
        if (is_check_stop == true || file_idx >= file_num) {
            int left = --left_thread_num;
            LEVIN_CINFO_LOG("Thread VerifyFileProcess end, left %d threads.", left);
            return;
        }

        std::map<std::string, std::string>::iterator it = md5_file_map.begin();
        for (int i = 0; i < file_idx; ++i) {
            ++it;
        }
        std::string  list_file_name = it->first;
        std::string  list_file_md5  = it->second;
        LEVIN_CINFO_LOG(
                "Thread VerifyFileProcess file [%s], md5 [%s]",
                list_file_name.c_str(),
                list_file_md5.c_str());

        if (!check_func(list_file_name, list_file_md5)) {
            LEVIN_CWARNING_LOG(
                    "Thread VerifyFileProcess failed, GetFileMD5 [%s] error",
                    list_file_name.c_str());
            is_check_stop = true;
            {
                boost::mutex::scoped_lock lock(cmutex);
                check_md5_list_result[file_idx] = false;
            }
            continue;
        } else {
            {
                boost::mutex::scoped_lock lock(cmutex);
                check_md5_list_result[file_idx] = true;
            }
            {
                boost_unique_lock lock(_wr_lock_global);
                _has_checked_file_list.insert(list_file_name);
            }
        }
    }
}

int SharedContainerManager::VerifyFiles(const std::map<std::string, std::string> verify_data,
        VerifyFileFuncPtr check_func, const int app_id) {
    int ret;
    std::map<std::string, std::string> diff_list;
    {
        boost_unique_lock lock(_wr_lock_global);
        for (auto ptr = verify_data.begin(); ptr != verify_data.end(); ptr++) {
            std::string absolute_path;
            ret = GetAbsolutePath(ptr->first, absolute_path);
            CHECK_RET(ret);
            
            _has_checked_file_list.erase(ptr->first);   //可能重复加载同一份数据
            std::pair<std::string, VerifyFileFuncPtr> value(ptr->second, check_func);
            _file_check_map[absolute_path] = value;
            diff_list[absolute_path] = ptr->second;
        }
    }

    std::vector<SharedMidInfo> global_mem_list;
    if (!SharedMemory::get_all_shmid(global_mem_list, false)) {
        LEVIN_CWARNING_LOG("Get current program path err!");
        return SC_RET_ERR_SYS;
    }

    for (auto ptr = global_mem_list.begin(); ptr != global_mem_list.end(); ptr++) {
        if (app_id == ptr->appid) {
            diff_list.erase(ptr->path);
        }
    }

    uint32_t thread_num;
    int cpu_num = sysconf(_SC_NPROCESSORS_CONF);
    thread_num = (cpu_num / 2) < 1 ? 1 : (cpu_num / 2);
    if (thread_num > diff_list.size()) {
        thread_num = diff_list.size();
    }
    LEVIN_CDEBUG_LOG("CheckDataMD5 cpu_num[%d] thread_num[%u]", cpu_num, thread_num);

    boost::atomic<int> processed_file_idx(0);
    boost::atomic<int> left_thread_num(thread_num);
    boost::atomic<bool> is_check_stop(false);

    boost::mutex check_mutex;
    std::vector<bool> check_md5_list_result(diff_list.size());
    for (size_t i = 0; i < check_md5_list_result.size(); ++i) {
        check_md5_list_result[i] = false;
    }

    std::vector<boost::shared_ptr<boost::thread> > check_threads;
    for (uint32_t i = 0; i < thread_num; ++i) {
        check_threads.push_back(boost::make_shared<boost::thread>(
            boost::bind(&SharedContainerManager::VerifyFileProcess,
                        boost::ref(diff_list),
                        boost::ref(processed_file_idx),
                        boost::ref(left_thread_num),
                        boost::ref(is_check_stop),
                        boost::ref(check_md5_list_result),
                        boost::ref(check_mutex),
                        check_func)));
    }

    for (uint32_t i = 0; i < thread_num; ++i) {
        check_threads[i]->join();
    }

    for (size_t i = 0; i < check_md5_list_result.size(); ++i) {
        if (check_md5_list_result[i] == false) {
            LEVIN_CWARNING_LOG("CheckDataMD5 failed, check file [%lu] false", i);
            return SC_RET_FILE_CHECK;
        }
    }

    return SC_RET_OK;
}

int SharedContainerManager::ClearByFileList(
        const std::set<std::string> &reserve_files, const int app_id) {
    std::set<std::string> absolute_files;
    for (auto ptr = reserve_files.begin(); ptr != reserve_files.end(); ptr++) {
        std::string absolute_path;
        int ret = GetAbsolutePath(*ptr, absolute_path);
        CHECK_RET(ret);
        absolute_files.insert(absolute_path);
    }

    std::vector<SharedMidInfo> global_mem_list;
    if (!SharedMemory::get_all_shmid(global_mem_list)) {
        LEVIN_CWARNING_LOG("Get current program path err!");
        return SC_RET_ERR_SYS;
    }

    for (auto ptr = global_mem_list.begin(); ptr != global_mem_list.end(); ptr++) {
        if ((app_id == ptr->appid) && absolute_files.find(ptr->path) == absolute_files.end()) {
            _has_checked_file_list.erase(ptr->path);
            SharedMemory::remove_shared_memory(ptr->mid);
        }
    }
    LEVIN_CINFO_LOG("ClearByFileList success!");
    return SC_RET_OK;
}

int SharedContainerManager::ClearByGroup(
        const std::set<std::string> &reserve_groups, const int app_id) {
    std::vector<SharedMidInfo> global_mem_list;
    if (!SharedMemory::get_all_shmid(global_mem_list)) {
        LEVIN_CWARNING_LOG("Get current program path err!");
        return SC_RET_ERR_SYS;
    }

    for (auto ptr = global_mem_list.begin(); ptr != global_mem_list.end(); ptr++) {
        if (app_id == ptr->appid && reserve_groups.find(ptr->groupid) == reserve_groups.end()) {
            _has_checked_file_list.erase(ptr->path);
            SharedMemory::remove_shared_memory(ptr->mid);
        }
    }
    LEVIN_CINFO_LOG("ClearByVersion success!");
    return SC_RET_OK;
}

int SharedContainerManager::ClearUnregistered(const int app_id) {
    std::vector<SharedMidInfo> global_mem_list;
    if (!SharedMemory::get_all_shmid(global_mem_list)) {
        LEVIN_CINFO_LOG("system err, get_all_shmid failed");
        return SC_RET_ERR_SYS;
    }

    {
        boost_unique_lock lock(_wr_lock_global);
        for (auto ptr = global_mem_list.begin(); ptr != global_mem_list.end(); ptr++) {
            if (app_id == ptr->appid 
                    && _global_container_map.find(ptr->path) == _global_container_map.end()) {
                _has_checked_file_list.erase(ptr->path);
                SharedMemory::remove_shared_memory(ptr->mid);
            }
        }
    }
    LEVIN_CINFO_LOG("ClearUnregistered success!");
    return SC_RET_OK;
}

int SharedContainerManager::VerifyOneFile(const std::string &file_path) {
    VerifyFileFuncPtr func;
    std::string check_msg;
    {
        boost_share_lock lock(_wr_lock_global);
        if (_has_checked_file_list.find(file_path) != _has_checked_file_list.end()) {
            return SC_RET_OK;
        }
        if (_file_check_map.find(file_path) == _file_check_map.end()) {
            return SC_RET_OK;
        }
        func = _file_check_map[file_path].second;
        check_msg = _file_check_map[file_path].first;
    }
    if (func(file_path, check_msg)) {
        boost_unique_lock lock(_wr_lock_global);
        _has_checked_file_list.insert(file_path);
        return SC_RET_OK;
    } else {
        LEVIN_CWARNING_LOG("verify failed, file path=[%s]", file_path.c_str());
        return SC_RET_FILE_CHECK;
    }
}

void SharedContainerManager::ClearSharedContainerProcess() {
    while(_clear_process_run) {
        {
            boost_unique_lock lock(_wr_lock_global);
            std::vector<std::string> deletePath;
            for (auto ptr = _global_container_map.begin(); ptr != _global_container_map.end(); ptr++) {
                if (ptr->second.second == STATUS_RELEASING) {
                    if (ptr->second.first.unique()) {
                        ptr->second.first->Destroy();
                        deletePath.push_back(ptr->first);
                        LEVIN_CWARNING_LOG("Destroy shared container, path=[%s]", ptr->first.c_str());
                    } else {
                        LEVIN_CWARNING_LOG("Destroy shared container failed, use_count!=1, path=[%s]",
                                ptr->first.c_str());
                    }
                } else if (ptr->second.second == STATUS_DELETING) {
                    deletePath.push_back(ptr->first);
                }
            }
            for (auto ptr = deletePath.begin(); ptr != deletePath.end(); ptr++) {
                _global_container_map.erase(*ptr);
                _has_checked_file_list.erase(*ptr);
                _file_check_map.erase(*ptr);
            }
        }
        boost::posix_time::milliseconds dura(1000);
        boost::this_thread::sleep(dura);
    }
}

void SharedContainerManager::StartClearProcess() {
    boost_unique_lock lock(_wr_lock_global);
    if (_clear_process_run == false) {
        _clear_process_run = true;
        _clear_process = boost::make_shared<boost::thread>(
                boost::thread(ClearSharedContainerProcess));
    }
}

void SharedContainerManager::StopClearProcess() {
    boost_unique_lock lock(_wr_lock_global);
    _clear_process_run = false;
    if (_clear_process) {
        _clear_process->join();
    }
}

}  // namespace levin


