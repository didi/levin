#ifndef __LEVIN_BENCHMARK_HEADER_H__
#define __LEVIN_BENCHMARK_HEADER_H__

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <malloc.h>
#include <boost/lexical_cast.hpp>
#include <boost/interprocess/managed_xsi_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/map.hpp>

namespace levin {

using namespace boost::interprocess;
typedef boost::interprocess::allocator<void, managed_xsi_shared_memory::segment_manager> VoidAllocator;
typedef boost::interprocess::allocator<int32_t, managed_xsi_shared_memory::segment_manager> IntAllocator;
typedef boost::interprocess::vector<int32_t, IntAllocator> IntVector;
typedef boost::interprocess::set<int32_t, std::less<int32_t>, IntAllocator> IntSet;
typedef std::pair<const int64_t, int32_t> Int64Int32Pair;
typedef boost::interprocess::allocator<Int64Int32Pair, managed_xsi_shared_memory::segment_manager> Int64Int32MapAllocator;
typedef boost::interprocess::map<int64_t, int32_t, std::less<int64_t>, Int64Int32MapAllocator> Int64Int32Map;

// @brief timer class (steal from its-lib)
class Timer {
public:
    Timer() {
        gettimeofday(&ts, NULL);
    }
    ~Timer() {
    }
    uint32_t get_time_us() {
        (void)gettimeofday(&te, NULL);
        return (uint32_t)(te.tv_sec - ts.tv_sec) * 1000 * 1000 + (te.tv_usec - ts.tv_usec);
    }

private:
    timeval ts;  // start time
    timeval te;  // end time
};

// @brief memory allocation information diff class (impl by MALLINFO(3))
// fields of structure mallinfo defined as follows:
//    int arena;     /* Non-mmapped space allocated (bytes) */
//    int ordblks;   /* Number of free chunks */
//    int smblks;    /* Number of free fastbin blocks */
//    int hblks;     /* Number of mmapped regions */
//    int hblkhd;    /* Space allocated in mmapped regions (bytes) */
//    int usmblks;   /* Maximum total allocated space (bytes) */
//    int fsmblks;   /* Space in freed fastbin blocks (bytes) */
//    int uordblks;  /* Total allocated space (bytes) */
//    int fordblks;  /* Total free space (bytes) */
//    int keepcost;  /* Top-most, releasable space (bytes) */
class MallocDiffer {
public:
    MallocDiffer() {
        start = mallinfo();
    }
    ~MallocDiffer() {
    }
    std::string get_diff_report_kb() {
        struct mallinfo end = mallinfo();
        std::stringstream ss;
        ss << "-------- malloc info diff report --------" << std::endl
           << "\tnon-mmap-total(kB)\tmmap(kB)\talloc(kB)\tfree(kB)" << std::endl
           << "now\t"
           << end.arena / 1024 << "\t\t"
           << end.hblkhd / 1024 << "\t\t"
           << end.uordblks / 1024 << "\t\t"
           << end.fordblks / 1024 << std::endl
           << "diff\t"
           << (end.arena - start.arena) / 1024 << "\t\t"
           << (end.hblkhd - start.hblkhd) / 1024 << "\t\t"
           << (end.uordblks - start.uordblks) /1024 << "\t\t"
           << (end.fordblks - start.fordblks) /1024 << std::endl
           << "--------------- report end --------------";
        return ss.str();
    }
private:
    struct mallinfo start;  // malloc info of start time
};

// @brief memory usage estimator class (impl by read proc statm)
class MemDiffer {
public:
    MemDiffer() {
        read_proc_memory(start);
    }
    ~MemDiffer() {
    }
    std::string get_diff_report_kb() {
        ProcMemory end;
        read_proc_memory(end);
        std::stringstream ss;
        ss << "-------- proc memory diff report --------" << std::endl
           << "\tsize(kB)\trss(kB)\tshare(kB)" << std::endl
           << "statm\t"
           << end.size * 4 << "\t\t" << end.rss * 4 << "\t\t" << end.share * 4 << std::endl
           << "diff\t"
           << (end.size - start.size) * 4 << "\t\t"
           << (end.rss - start.rss) * 4 << "\t\t"
           << (end.share - start.share) * 4 << std::endl
           << "--------------- report end --------------";
        return ss.str();
    }

private:
    // @brief proc memory struct
    struct ProcMemory {
        long size;      // total program size(pages) VmSize/4
        long rss;       // resident set size(pages) VmRSS/4
        long share;     // shared pages
        long trs;       // text (code)
        long lrs;       // library
        long drs;       // data/stack
        long dt;        // dirty pages
    };
    bool read_proc_memory(ProcMemory &m);

    ProcMemory start;  // proc memory of start time
};

inline bool MemDiffer::read_proc_memory(ProcMemory &m) {
    std::string stat_file = "/proc/" + boost::lexical_cast<std::string>(getpid()) + "/statm";
    FILE *fp = fopen(stat_file.c_str(), "r");
    if (fp == nullptr) {
        std::cout << "fail to open " << stat_file << std::endl;
        return false;
    }
    if (fscanf(fp, "%ld %ld %ld %ld %ld %ld %ld",
                &m.size, &m.rss, &m.share, &m.trs, &m.lrs, &m.drs, &m.dt) != 7) {
        std::cout << "fail to fscanf " << stat_file << std::endl;
        fclose(fp);
        return false;
    }
    fclose(fp);
    return true;
}

}  // namespace levin

#endif  // __LEVIN_BENCHMARK_HEADER_H__
