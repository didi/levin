#ifndef LEVIN_TIMER_H
#define LEVIN_TIMER_H

#include <stdint.h>
#include <sys/time.h>
#include "levin_logger.h"

namespace levin {

class Timer {
public:
    Timer() {
        (void)gettimeofday(&ts, NULL);
    }
    ~Timer() {}
    
    uint32_t get_time_s() {
        (void)gettimeofday(&te, NULL);
        return (uint32_t)(te.tv_sec - ts.tv_sec);
    }

    uint32_t get_time_ms() {
        (void)gettimeofday(&te, NULL);

        return (uint32_t)(te.tv_sec - ts.tv_sec) * 1000 + (te.tv_usec - ts.tv_usec) / 1000;
    }

    uint32_t get_time_us() {
        (void)gettimeofday(&te, NULL);

        return (uint32_t)(te.tv_sec - ts.tv_sec) * 1000 * 1000 + (te.tv_usec - ts.tv_usec);
    }

private:
    timeval ts;
    timeval te;
};

class TimerGuard {
public:
    TimerGuard(const std::string &path, const std::string &func): _timer(Timer()), _path(path), _func(func) {}
    ~TimerGuard() {
        double latency_ms = (double)_timer.get_time_ms();
        LEVIN_CINFO_LOG("step=[func=%s, path=%s], time_cost=[%.1f s]", _func.c_str(), _path.c_str(), latency_ms / 1000.0);
    }
private:
    Timer _timer;
    std::string _path;
    std::string _func;
};

}

#endif
