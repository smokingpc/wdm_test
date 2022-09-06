#include "pch.h"

#define     LOG_SIZE        1048576
#define     TAG_LOG         (ULONG)'SGOL'
#define     TINYBUF_SIZE    32

EXT_CALLBACK ExTimerCallback;
PEX_TIMER TimerHandle1 = NULL;
CHAR* LogData = NULL;

void ExtCallback(PEX_TIMER Timer, PVOID Context)
{
    UNREFERENCED_PARAMETER(Timer);
    CHAR temp[TINYBUF_SIZE] = {0};
    LARGE_INTEGER freq = { 0 };
    LARGE_INTEGER timestamp = KeQueryPerformanceCounter(&freq);
    CHAR* buffer = (CHAR*)Context;
    RtlStringCbPrintfA(temp, TINYBUF_SIZE, "[counter] %lld\n", timestamp.QuadPart);
    RtlStringCbCatA(buffer, LOG_SIZE, temp);
}

EXTERN_C_START
DRIVER_INITIALIZE   DriverEntry;

NTSTATUS __stdcall
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    DbgBreakPoint();

    LogData = (CHAR *)ExAllocatePoolWithTag(NonPagedPool, LOG_SIZE, TAG_LOG);
    TimerHandle1 = ExAllocateTimer(ExtCallback, LogData, EX_TIMER_HIGH_RESOLUTION);

    if(NULL != TimerHandle1)
    {
        EXT_SET_PARAMETERS param = {0};
        ExInitializeSetTimerParameters(&param);

        //indicates CPU should wakeup immediately even got timer in low power state.
        param.NoWakeTolerance = 0;

        //set timer due time 100us and period 100us loop.
        ExSetTimer(TimerHandle1, -10*100, 10*100, &param);
    }

    DbgBreakPoint();
    LARGE_INTEGER interval = {0};
    interval.QuadPart = -10*1000*1000*30;        //30 seconds.
    KeDelayExecutionThread(KernelMode, FALSE, &interval);
    DbgBreakPoint();

    if (NULL != TimerHandle1)
    {
        EXT_DELETE_PARAMETERS param = {0};
        ExInitializeDeleteTimerParameters(&param);
        ExDeleteTimer(TimerHandle1, TRUE, TRUE, &param);
    }
    if(NULL != LogData)
        ExFreePoolWithTag(LogData, TAG_LOG);

    return STATUS_INTERNAL_ERROR;
}

EXTERN_C_END
