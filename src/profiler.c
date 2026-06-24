#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <psapi.h>

double GetProcessMemoryUsage(void) {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return (double)pmc.WorkingSetSize / (1024.0 * 1024.0); // Convert to MB
    }
    return 0.0;
}
#else
double GetProcessMemoryUsage(void) {
    return 0.0;
}
#endif
