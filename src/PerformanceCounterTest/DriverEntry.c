#include <wdm.h>

DRIVER_INITIALIZE   DriverEntry;

NTSTATUS __stdcall
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    LARGE_INTEGER freq = {0};
    LARGE_INTEGER counter = KeQueryPerformanceCounter(&freq);

    ULONG maxt, mint, curt;
    ExQueryTimerResolution(&maxt, &mint, &curt);
    LARGE_INTEGER tick;
    LARGE_INTEGER tick2;
    KeQueryTickCount(&tick);
    LARGE_INTEGER counter2 = KeQueryPerformanceCounter(&freq);
    LARGE_INTEGER counter3 = KeQueryPerformanceCounter(&freq);

    KeQueryTickCount(&tick2);
    DbgBreakPoint();

    UNREFERENCED_PARAMETER(counter);
    UNREFERENCED_PARAMETER(counter2);
    UNREFERENCED_PARAMETER(counter3);

    return STATUS_INTERNAL_ERROR;
}

