#ifndef LEVIN_SHARED_UTILS_H
#define LEVIN_SHARED_UTILS_H

#include <sstream>
#include <cstring>
#include <cxxabi.h>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <openssl/md5.h>
#include "levin_logger.h"

namespace levin {

// @brief shared container ret code
enum {
    SC_RET_OK = 0,              // 成功
    SC_RET_FILE_NOEXIST,        // 文件不存在
    SC_RET_SHM_SIZE_ERR,        // SHM内存SIZE错误
    SC_RET_OOM,                 // OOM, SHM分配失败
    SC_RET_READ_FAIL,           // 读文件失败
    SC_RET_ALLOC_FAIL,          // SHM寻址或构造失败
    SC_RET_CHECK_FAIL,          // SHM内存校验失败
    SC_RET_LOAD_FAIL,           // 加载文件失败
    SC_RET_LOADING,             // 其他相同容器正在加载
    SC_RET_ERR_TYPE,            // 容器类型不匹配
    SC_RET_NO_REGISTER,         // 容器未注册
    SC_RET_HAS_REGISTED,        // 容器已注册过
    SC_RET_ERR_STATUS,          // 容器状态异常
    SC_RET_FILE_CHECK,          // 文件校验出错
    SC_RET_EXCEPTION,           // 有异常抛出
    SC_RET_ERR_SYS              // 获取系统信息出错
};

inline const char* CodeToMsg(int code) {
    static const std::unordered_map<int, const char*> code_msg_pairs = {
        {SC_RET_OK,             "OK"                                               },
        {SC_RET_FILE_NOEXIST,   "File no exist"                                    },
        {SC_RET_SHM_SIZE_ERR,   "Shm alloc size error, 0 or >60G been forbidden"   },
        {SC_RET_OOM,            "Out of memory"                                    },
        {SC_RET_READ_FAIL,      "File read error, fail or bad"                     },
        {SC_RET_ALLOC_FAIL,     "Allocate in shm region fail, out of range"        },
        {SC_RET_CHECK_FAIL,     "Exist shm check fail"                             },
        {SC_RET_LOAD_FAIL,      "File load error, bad or validate fail"            },
        {SC_RET_LOADING,        "Duplicate container is loading"                   },
        {SC_RET_ERR_TYPE,       "Error container type"                             },
        {SC_RET_NO_REGISTER,    "Unregistered container"                           },
        {SC_RET_HAS_REGISTED,   "Duplicate container has registed by this manager" },
        {SC_RET_ERR_STATUS,     "Container can't be used right now"                },
        {SC_RET_FILE_CHECK,     "File MD5 check fail"                              },
        {SC_RET_EXCEPTION,      "Container internal exception"                     },
        {SC_RET_ERR_SYS,        "Xsi share memory system error"                    }
    };
    auto ret = code_msg_pairs.find(code);
    if (ret != code_msg_pairs.end()) {
        return ret->second;
    }
    return "Invalid error code";
}

inline std::string demangle(const char *name) {
    int status = 0;
    char *buf = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    if (status == 0) {
        std::string readable(buf);
        free(buf);
        return readable;
    }
    return std::string(name);
}

// @brief shared container meta struct typedef
// eg. [ ./road.data -> CustomVector<uint32_t> ]
// meta.path     = ./road.dat                        container file path
// meta.summary  = CustomVector<unsigned int>        demangled container typeid name, for readable show
// meta.hashcode = 17751363617614597197              container typeid hash, for compare
// meta.flags    = 72057594037927936                 container flags(version etc.)
// meta.label    = 4886718345                        magic num:0x123456789, for notify container integrity
// meta.checksum = 626738dd02fccd46b2d9ed8d093014d7  container memory region checksum, for one-by-one byte check
const size_t PATH_LENGTH = 1024;
const size_t TYPENAME_ID_LENGTH = 128;
const size_t GROUP_ID_LENGTH = 128;
const size_t MD5SUM_DIGEST_LENGTH = 16;
typedef struct Meta {
    char path[PATH_LENGTH + 1];
    uint64_t flags;
    char group[GROUP_ID_LENGTH + 1];
    int id;
    char summary[TYPENAME_ID_LENGTH + 1];
    uint64_t hashcode;
    uint64_t label;
    char checksum[MD5SUM_DIGEST_LENGTH * 2 + 1];
    Meta() : flags(0), hashcode(0), label(0) {
        memset(path, 0, sizeof(path));
        memset(summary, 0, sizeof(path));
        memset(checksum, 0, sizeof(checksum));
    }
    Meta(const char *name, const char *type, const char *group_name,
         const int appid, const size_t hash, const uint64_t flg) :
            flags(flg), id(appid), hashcode(hash), label(0) {
        strncpy(path, name, PATH_LENGTH);
        path[PATH_LENGTH] = '\0';
        strncpy(summary, demangle(type).c_str(), TYPENAME_ID_LENGTH);
        summary[TYPENAME_ID_LENGTH] = '\0';
        strncpy(group, group_name, GROUP_ID_LENGTH);
        group[GROUP_ID_LENGTH] = '\0';
        memset(checksum, 0, sizeof(checksum));
    }
    std::string layout() const;
} SharedMeta;

typedef struct MidInfo {
    std::string path;
    int mid;
    std::string groupid;
    int appid;
    MidInfo() : path(""), mid(0), groupid(""), appid(0) {}
    MidInfo(const std::string file_path, const int mem_id, const std::string group_id, const int app_id) :
            path(file_path), mid(mem_id), groupid(group_id), appid(app_id) {}
} SharedMidInfo;

inline std::string SharedMeta::layout() const {
    std::stringstream ss;
    ss << "[" << (void*)&path << "]\t\tmeta.path=" << path << std::endl
       << "[" << (void*)&summary << "]\t\tmeta.summary=" << summary << std::endl
       << "[" << (void*)&hashcode << "]\t\tmeta.hashcode=" << hashcode << std::endl
       << "[" << (void*)&flags << "]\t\tmeta.flags=" << flags << std::endl
       << "[" << (void*)&label << "]\t\tmeta.label=" << label << std::endl
       << "[" << (void*)&checksum << "]\t\tmeta.checksum=" << checksum;
    return ss.str();
}

inline std::ostream& operator<<(std::ostream &os, const SharedMeta &meta) {
    return os << meta.layout();
}

#define LEVIN_INLINE inline __attribute__((always_inline))

#define CHECK_FILE_READ_OR_WRITE_RES(handle, file)      \
    do {                                                \
        if (!handle) {                                   \
            handle.close();                             \
            LEVIN_CWARNING_LOG("read file fail %s",file.c_str());\
            return false;                               \
        }                                               \
    } while (0);

#define SAFE_DELETE(pointer)                    \
    do {                                        \
        if (NULL != pointer) {                  \
            delete pointer;                     \
            pointer = NULL;                     \
       }                                        \
    } while (0);

static const uint64_t prime[] = {
    17, 37, 79, 163, 331, 673, 1361, 2729,
    5471, 10949, 21911, 43853, 87719, 175447, 350899, 701819, 1403641,
    2807303, 5614657, 11229331, 22458671, 44917381, 89834777, 179669557,
    359339171, 718678369, 1437356741, 2147483647};

static std::vector<uint32_t> prime_table(prime, prime + sizeof(prime) / sizeof(prime[0]));

inline uint64_t getPrime(uint64_t i) {
    auto it = std::upper_bound(prime_table.begin(), prime_table.end(), i);
    if (it == prime_table.end()) {
        i = *(prime_table.rbegin());
    } else {
        i = *it;
    }
    return i;
}

// @brief shm checksum struct typedef
typedef struct ChecksumInfo {
    void *area;
    unsigned long length;
    ChecksumInfo() : area(nullptr), length(0) {
    }
    ChecksumInfo(void *ptr, const unsigned long len) : area(ptr), length(len) {
    }
} SharedChecksumInfo;

typedef std::function<bool(const ChecksumInfo&, SharedMeta*, bool)> CheckFunctor;

class Md5Checker {
public:
    bool operator()(const ChecksumInfo &info, SharedMeta *meta, bool is_upd = false) {
        LEVIN_CDEBUG_LOG("Md5Checker. area=%p, len=%ld", info.area, info.length);
        char cur_sum[MD5SUM_DIGEST_LENGTH * 2 + 1] = {0};
        checksum(info.area, info.length, cur_sum);
        if (is_upd) {
            strncpy(meta->checksum, cur_sum, sizeof(cur_sum));
            return true;
        }
        return strncasecmp(cur_sum, meta->checksum, sizeof(cur_sum)) == 0;
    }
protected:
    void checksum(const void *ptr, unsigned long len, char *sum) {
        MD5_CTX ctx;
        MD5_Init(&ctx);
        MD5_Update(&ctx, ptr, len);
        unsigned char md5_final[MD5SUM_DIGEST_LENGTH];
        MD5_Final(md5_final, &ctx);

        for (size_t i = 0; i < MD5SUM_DIGEST_LENGTH; ++i) {
            snprintf(&sum[2 * i], 3, "%02x", md5_final[i]);
        }
        sum[MD5SUM_DIGEST_LENGTH * 2] = '\0';
        LEVIN_CDEBUG_LOG("Md5Checker. calc checksum=%s", sum);
    }
};

class IntegrityChecker {
public:
    bool operator()(const ChecksumInfo &info, SharedMeta *meta, bool is_upd = false) {
        LEVIN_CDEBUG_LOG("IntegrityChecker. area=%p, len=%ld", info.area, info.length);
        if (is_upd) {
            meta->label = label_magic_num;
            return true;
        }
        return meta->label == label_magic_num;
    }
protected:
    static const uint64_t label_magic_num = 0x123456789;
};

template <class Key, class Value>
bool CMP(const std::pair<Key, Value> &a, const std::pair<Key, Value> &b) {
    return a.first < b.first;
}

template <class Key, class Value>
static int32_t binary_search(const std::pair<Key, Value>* data, int right, Key key) {
    int left = 0, mid;
    while (left <= right) {
        mid = (left + right) / 2;
        if (data[mid].first == key) {
            return mid;
        }
        if (data[mid].first < key) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return -1;
}

template <class Key, class Value, class Compare>
struct PairCompare {
    bool operator() (const std::pair<Key, Value> &data, const Key &key) const {
        return _comp(data.first, key);
    }
    bool operator() (const Key &key, const std::pair<Key, Value> &data) const {
        return _comp(key, data.first);
    }
    Compare _comp;
};

// @brief shared container file header struct typedef
typedef struct Header {
    uint64_t container_size;
    uint64_t type_hash;
    // unsigned 8-bit version + reserved 56-bit(padding with 0)
    uint64_t flags;
    // other file header field...
} SharedFileHeader;

inline uint8_t VersionOfFlags(const uint64_t flags) {
    return (uint8_t)(flags >> 56);
}
inline uint64_t makeFlags(uint8_t version) {
    return (uint64_t)(((uint64_t)version << 56) | 0x0);
}

}  // namespace levin

#endif  // LEVIN_SHARED_UTILS_H
