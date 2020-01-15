#ifndef LEVIN_XSI_SHARED_MEMORY_H
#define LEVIN_XSI_SHARED_MEMORY_H

#include <sys/stat.h>
#include <sys/shm.h>
#include <errno.h>
#include <iostream>
#include <boost/noncopyable.hpp>
#include "levin_logger.h"

namespace levin {

// @brief XSI shm create mode enum typedef
enum class XsiShmCreateMode {
    Create,
    Open,
    OpenOrCreate
};

// @brief XSI shm class
class XsiSharedMemory : public boost::noncopyable {
public:
    XsiSharedMemory() {
    }
    ~XsiSharedMemory() {
    }

    // @brief allocates a shared memory segment
    // retval succ: 0 fail: errno (Never throws)
    int open(XsiShmCreateMode mode, key_t key, size_t size = 0, int shmperm = 0644);

    // @biref erase the XSI shared memory object identified by shmid from the system
    // retval succ: true fail: false (Never throws)
    static bool remove(int shmid) { return shmctl(shmid, IPC_RMID, nullptr) != -1; }

    int get_shmid() const { return _shmid; }

protected:
    int _shmid = -1;
};

inline int XsiSharedMemory::open(XsiShmCreateMode mode, key_t key, size_t size, int shmperm) {
    int shmflg = shmperm;
    shmflg &= 0x01FF;
    switch (mode) {
    case XsiShmCreateMode::Open:
        shmflg |= 0;
        break;
    case XsiShmCreateMode::Create:
        shmflg |= IPC_CREAT | IPC_EXCL;
        break;
    case XsiShmCreateMode::OpenOrCreate:
        shmflg |= IPC_CREAT;
        break;
    default:
        LEVIN_CWARNING_LOG("Invalid xsi shm create mode, mode=%d", mode);
        return -1;
    }

    int ret = shmget(key, size, shmflg);
    int shmid = ret;
    shmid_ds shm_ds;
    if((shmid != -1) && (mode == XsiShmCreateMode::Open)) {
        ret = shmctl(shmid, IPC_STAT, &shm_ds);
    }
    if (ret == -1) {
        return errno;
    }
    _shmid = shmid;
    return 0;
}

// @brief shm mapped region class
class MappedRegion : public boost::noncopyable {
public:
    MappedRegion(int shmid, int mode = 0) :
            _shmid(shmid), _mode(mode), _address(nullptr), _size(0) {
    }
    ~MappedRegion() {
        detach();
    }

    // @brief attaches the shared memory segment identified by shmid
    // to the address space of the calling process
    // retval succ: 0 fail: errno (Never throws)
    int attach();
    int detach(); 

    void* get_address() const { return _address; }
    size_t get_size() const { return _size; }

private:
    int _shmid = -1;
    int _mode = 0;
    void *_address = nullptr;
    size_t _size = 0;
};

inline int MappedRegion::attach() {
    shmid_ds shm_ds;
    if (shmctl(_shmid, IPC_STAT, &shm_ds) == -1) {
        return errno;
    }
    // If SHM_RDONLY is specified in shmflg, the segment is attached for reading
    // Otherwise the segment is attached for read and write
    // There is no notion of a write-only shared memory segment
    int flag = 0;
    if (_mode == SHM_RDONLY) {
        flag |= SHM_RDONLY;
    }
    void *base = shmat(_shmid, (void*)nullptr, flag);
    if (base == (void*)-1) {
        std::cout << "shmat failed, shmid=" << _shmid << ", errno=" << errno << std::endl;
        return errno;
    }

    _address = base;
    _size = shm_ds.shm_segsz;
    return 0;
}

inline int MappedRegion::detach() {
    if (_address != nullptr) {
        if (shmdt(_address) != 0) {
            return errno;
        }
    }
    _address = nullptr;
    return 0;
}

}  // namespace levin

#endif  // LEVIN_XSI_SHARED_MEMORY_H
