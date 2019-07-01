#ifndef  LEVIN_SHARED_MANAGER_H
#define  LEVIN_SHARED_MANAGER_H

#include "xsi_shm.hpp"
#include "check_file.h"
#include "shared_utils.h"
#include "levin_logger.h"
#include <string>
#include <memory>
#include <map>
#include <set>
#include <exception>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp> 
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include "shared_base.hpp"

namespace levin {

typedef bool (*VerifyFileFuncPtr)(const std::string, const std::string);

#define CHECK_RET(ret) \
    if (ret != SC_RET_OK) { \
        return ret; \
    }

class SharedManagerGuard {
public:
    SharedManagerGuard();
    ~SharedManagerGuard();

private:
    SharedManagerGuard(const SharedManagerGuard &rhs);
    SharedManagerGuard& operator = (const SharedManagerGuard& rhs);
};

class SharedContainerManager {
public:
    enum SharedContainerStatus {
        STATUS_LOADING,
        STATUS_READY,
        STATUS_DELETING,
        STATUS_RELEASING
    };
    typedef boost::shared_lock<boost::shared_mutex> boost_share_lock;
    typedef boost::unique_lock<boost::shared_mutex> boost_unique_lock;
    typedef std::pair<std::shared_ptr<SharedBase>, SharedContainerStatus> ptr_status_pair;
    typedef std::pair<std::string, VerifyFileFuncPtr> auth_func_pair;

    friend class SharedManagerGuard;

    SharedContainerManager(const std::string group_name,  const int app_id = 1);
    ~SharedContainerManager();

    template <typename T>
    int Register(const std::string &file_path, std::shared_ptr<T> &container_ptr) {    //注册并获取容器指针
        int ret;
        std::string absolute_path;
        ret = GetAbsolutePath(file_path, absolute_path);
        CHECK_RET(ret);

        try {
            container_ptr.reset(new T(absolute_path, _group_name, _app_id));
            if (container_ptr == nullptr) {
                LEVIN_CWARNING_LOG("creat new container failed, file path=[%s]", file_path.c_str());
                return SC_RET_OOM;
            }
            ret = AddLoading(absolute_path, container_ptr);
            CHECK_RET(ret);
            {
                boost_unique_lock lock(_wr_lock_container_init);
                ret = container_ptr->Init();
            }
            if (ret != SC_RET_OK) {
                if (ret == SC_RET_OOM) {
                    ClearUnregistered();
                    ret = container_ptr->Init();
                }
                if (ret != SC_RET_OK) {
                    DeleteLoading(absolute_path);
                    LEVIN_CWARNING_LOG(
                            "container init failed, file path=[%s], ret=%d",
                            file_path.c_str(), ret);
                    return ret;
                }
            }

            if (!container_ptr->IsExist()) {
                ret = VerifyOneFile(absolute_path);
                CHECK_RET(ret);
                ret = container_ptr->Load();
                if (ret != SC_RET_OK) {
                    LEVIN_CWARNING_LOG(
                            "shared container load failed, file path=[%s]",
                            absolute_path.c_str());
                    container_ptr->Destroy();
                    container_ptr.reset();
                    DeleteLoading(absolute_path);
                    return ret;
                }
            }
        }
        catch (std::exception& e) {
            LEVIN_CWARNING_LOG(
                "exception happened when creat shared-container, file_path=[%s] msg=[%s]",
                file_path.c_str(), e.what());
            if (container_ptr) {
                container_ptr->Destroy();
                container_ptr.reset();
            }
            DeleteLoading(absolute_path);
            return SC_RET_EXCEPTION;
        }
        catch (...) {
            LEVIN_CWARNING_LOG(
                "exception happened when creat shared-container, file_path=[%s] msg=[unknown]",
                file_path.c_str());
            if (container_ptr) {
                container_ptr->Destroy();
                container_ptr.reset();
            }
            DeleteLoading(absolute_path);
            return SC_RET_EXCEPTION;
        }
        ret = UpdateSharedStatus(absolute_path, STATUS_READY);
        CHECK_RET(ret);
        LEVIN_CINFO_LOG(
                "register success, path=[%s], container size=%lu",
                file_path.c_str(), container_ptr->size());
        return SC_RET_OK;
    }

    template <typename T>
    static int GetContanerPtr(const std::string &file_path, std::shared_ptr<T> &container_ptr) {
        std::string absolute_path;
        int ret = GetAbsolutePath(file_path, absolute_path);
        CHECK_RET(ret);
        {
            boost_share_lock lock(_wr_lock_global);
            if (_global_container_map.find(absolute_path) != _global_container_map.end()) {
                if (_global_container_map[absolute_path].second != STATUS_READY) {
                    return SC_RET_ERR_STATUS;
                }
                container_ptr = std::dynamic_pointer_cast<T>(_global_container_map[absolute_path].first);
                if (container_ptr == nullptr) {
                    LEVIN_CWARNING_LOG(
                        "get container ptr with err type, file path=[%s]",
                        file_path.c_str());
                    return SC_RET_ERR_TYPE;
                }
                return SC_RET_OK;
            }
        }

        return SC_RET_NO_REGISTER;
    }

    void Release();

    static int VerifyFiles(const std::map<std::string, std::string> verify_data,
            VerifyFileFuncPtr check_func = CheckFileMD5, const int app_id = 1);

    static int ClearByFileList(const std::set<std::string> &reserve_files, const int app_id = 1);  

    static int ClearByGroup(const std::set<std::string> &reserve_groups, const int app_id = 1);

    static int ClearUnregistered(const int app_id = 1);

private:
    static int GetAbsolutePath(const std::string &file_path, std::string &absolute_path);

    int AddLoading(const std::string &key_path, std::shared_ptr<SharedBase> shared_ptr);

    int DeleteLoading(const std::string &key_path);

    void ReleaseContainer(const std::string &key_path);

    void DeleteContainer(const std::string &key_path);

    int UpdateSharedStatus(const std::string &key_path, const SharedContainerStatus status);

    int VerifyOneFile(const std::string &file_path);

    static void ClearSharedContainerProcess();

    static void VerifyFileProcess(std::map<std::string, std::string>& md5_file_map,
            boost::atomic<int>&  processed_file_idx,
            boost::atomic<int>&  left_thread_num,
            boost::atomic<bool>& is_check_stop,
            std::vector<bool>& check_md5_list_result,
            boost::mutex& cmutex,
            VerifyFileFuncPtr check_func);

    static void StartClearProcess();

    static void StopClearProcess();

    SharedContainerManager(const SharedContainerManager&) = delete;
    SharedContainerManager(SharedContainerManager&&) = delete;
    SharedContainerManager& operator =(const SharedContainerManager&) = delete;
    SharedContainerManager& operator =(SharedContainerManager&&) = delete;

    std::map<std::string, std::shared_ptr<SharedBase> > _local_container_map;
    std::string _group_name;
    int _app_id;
    static std::map<std::string, ptr_status_pair> _global_container_map;
    static std::map<std::string, auth_func_pair> _file_check_map;
    static std::set<std::string> _has_checked_file_list;
    static bool _clear_process_run;
    static boost::shared_ptr<boost::thread> _clear_process;

    //读写锁
    boost::shared_mutex _wr_lock_local;
    static boost::shared_mutex _wr_lock_global;
    static boost::shared_mutex _wr_lock_container_init;
};


}  // namespace levin

#endif  // LEVIN_SHARED_MANAGER_H
