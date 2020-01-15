#ifndef LEVIN_ID_MANAGER_H
#define LEVIN_ID_MANAGER_H

#include <mutex>
#include <boost/noncopyable.hpp>
#include "shared_utils.h"

namespace levin {

// @brief ID manager
// manage System V share memory segment identifier
class IdManager : public boost::noncopyable {
public:
    // @brief {shm id, shared container binfile path}
    typedef std::unordered_map<int, std::string> IdMap;
    // @brief {shared container binfile path, shm id}
    typedef std::unordered_map<std::string, int> ShmMap;

    bool Register(const int id, const std::string &name);
    bool DeRegister(const int id);
    bool GetId(const std::string &name, int &id) const;

    static IdManager& GetInstance() {
        static IdManager instance;
        return instance;
    }

    // @brief get_all_shmid maybe called before main(). logger by stdout
    static bool get_all_shmid(std::vector<SharedMidInfo> &shmid_info, bool no_attach);

private:
    // @brief This constructor does nothing more than ensure that instance is init before main()
    // thus creating the static object before multithreading race issues come up
    struct ObjectCreator {
        ObjectCreator() {
            IdManager::GetInstance().init();
        }
    };
    static ObjectCreator creator;

    // @brief init is called before main(). logger by stdout
    bool init();
    bool insertWithLock(const int id, const std::string &name);
    bool deleteWithLock(const int id);
    IdManager() {}

private:
    IdMap _id_map;
    ShmMap _shm_map;
    mutable std::mutex _mutex;
};

}  // namespace levin

#endif  // LEVIN_ID_MANAGER_H
