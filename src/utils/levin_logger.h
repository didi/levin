#ifndef LEVIN_LOGGER_H
#define LEVIN_LOGGER_H

namespace levin {

// @brief 0: debuglog turned off  1: debuglog turned on
#define LEVIN_DEBUG_LOG_LEVEL 1

#define LEVIN_CDEBUG_LOG(fmt, ...)                                      \
    if (LEVIN_DEBUG_LOG_LEVEL > 0) {                                    \
        fprintf(stdout, "[DEBUG][%sT%s][%s:%d] " fmt " \n",             \
                __DATE__, __TIME__, __FILE__, __LINE__, ##__VA_ARGS__); \
    }                                                                   \

#define LEVIN_CINFO_LOG(fmt, ...)    \
    do {                             \
        fprintf(stdout, "[INFO][%sT%s][%s:%d] " fmt " \n",              \
                __DATE__, __TIME__, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(false);                  \

#define LEVIN_CWARNING_LOG(fmt, ...) \
    do {                             \
        fprintf(stdout, "[WARN][%sT%s][%s:%d] " fmt " \n",              \
                __DATE__, __TIME__, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(false);                  \

#define LEVIN_CFATAL_LOG(fmt, ...)   \
    do {                             \
        fprintf(stderr, "[FATAL][%sT%s][%s:%d] " fmt " \n",             \
                __DATE__, __TIME__, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(false);                  \

}  // namespace levin

#endif  // LEVIN_LOGGER_H
