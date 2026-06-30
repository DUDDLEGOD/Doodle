#ifndef PROFILER_H
#define PROFILER_H

#include <stddef.h>

#if defined(_WIN32) || defined(_WIN64)

// Forward declare the bare minimum from Windows API to avoid including windows.h
typedef void* HANDLE;

typedef struct _PROCESS_MEMORY_COUNTERS {
    unsigned long cb;
    unsigned long PageFaultCount;
    size_t PeakWorkingSetSize;
    size_t WorkingSetSize;
    size_t QuotaPeakPagedPoolUsage;
    size_t QuotaPagedPoolUsage;
    size_t QuotaPeakNonPagedPoolUsage;
    size_t QuotaNonPagedPoolUsage;
    size_t PagefileUsage;
    size_t PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;

#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllimport) int __stdcall GetProcessMemoryInfo(HANDLE Process, PROCESS_MEMORY_COUNTERS* ppsmemCounters, unsigned long cb);
#ifdef __cplusplus
}
#endif

static inline double GetProcessMemoryUsage(void) {
    PROCESS_MEMORY_COUNTERS pmc;
    pmc.cb = sizeof(pmc);
    // GetCurrentProcess() is statically defined as (HANDLE)-1
    HANDLE current_process = (HANDLE)(long long)-1;
    if (GetProcessMemoryInfo(current_process, &pmc, sizeof(pmc))) {
        return (double)pmc.WorkingSetSize / (1024.0 * 1024.0); // Convert to MB
    }
    return 0.0;
}

#else

static inline double GetProcessMemoryUsage(void) {
    return 0.0;
}

#endif

#endif // PROFILER_H
