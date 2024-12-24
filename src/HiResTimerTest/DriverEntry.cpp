#include "pch.h"

#define     LOG_SIZE        1048576
#define     TAG_LOG         (ULONG)'SGOL'
#define     TINYBUF_SIZE    64
#define     RUNTIME_IN_EACH_TEST    (-10 * 1000 * 1000 * 10)    //10 sec
#define     HIGHRES_TIMER_INTERVAL  (-500 * 10)         //500us
#define     NOWAKE_TIMER_INTERVAL   (-2 * 1000 * 10)    //2ms
#define     NOWAKE_TIMER_TOLERANCE  (8 * 1000 * 10)     //8ms, negative tolerance time will cause bugcheck!

EXT_CALLBACK ExTimerCallback1;
EXT_CALLBACK ExTimerCallback2;
PEX_TIMER TimerHandle1 = NULL;
PEX_TIMER TimerHandle2 = NULL;
EXT_SET_PARAMETERS TimerParam1 = { 0 };
EXT_SET_PARAMETERS TimerParam2 = { 0 };

char LogData1[LOG_SIZE] = {0};
char *LogCursor1 = LogData1;
char LogData2[LOG_SIZE] = { 0 };
char* LogCursor2 = LogData2;

void ExTimerCallback1(PEX_TIMER Timer, PVOID Context)
{
    UNREFERENCED_PARAMETER(Timer);
    UNREFERENCED_PARAMETER(Context);

    LARGE_INTEGER freq = { 0 };
    LARGE_INTEGER timestamp = KeQueryPerformanceCounter(&freq);

    //even sprintf is unsafe, it is more convinent in simple test...
    int written = sprintf(LogCursor1, "(%lld MHz) %lld\n", freq.QuadPart, timestamp.QuadPart);
    LogCursor1 += written;

    ExSetTimer(TimerHandle1, HIGHRES_TIMER_INTERVAL, 0, &TimerParam1);
}
void TestHighResTimer()
{
    TimerHandle1 = ExAllocateTimer(ExTimerCallback1, NULL, EX_TIMER_HIGH_RESOLUTION);
    if (NULL != TimerHandle1)
    {
        ExInitializeSetTimerParameters(&TimerParam1);

        //indicates CPU should wakeup immediately even got timer in low power state.
        TimerParam1.NoWakeTolerance = 0;

        //set timer due time 500us.
        ExSetTimer(TimerHandle1, HIGHRES_TIMER_INTERVAL, 0, &TimerParam1);
    }

    LARGE_INTEGER interval = { 0 };
    interval.QuadPart = RUNTIME_IN_EACH_TEST / 10;      //shorten the time, or the log will be huge...
    KeDelayExecutionThread(KernelMode, FALSE, &interval);

    if (NULL != TimerHandle1)
    {
        EXT_DELETE_PARAMETERS param = { 0 };
        ExInitializeDeleteTimerParameters(&param);
        ExDeleteTimer(TimerHandle1, TRUE, TRUE, &param);
    }
}
void PrintHighResTimerLog()
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[HighResolution Timer Testing]\n");
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%s", LogData1);
}

void ExTimerCallback2(PEX_TIMER Timer, PVOID Context)
{
    UNREFERENCED_PARAMETER(Timer);
    UNREFERENCED_PARAMETER(Context);

    LARGE_INTEGER freq = { 0 };
    LARGE_INTEGER timestamp = KeQueryPerformanceCounter(&freq);

    //even sprintf is unsafe, it is more convinent in simple test...
    int written = sprintf(LogCursor2, "(%lld MHz) %lld\n", freq.QuadPart, timestamp.QuadPart);
    LogCursor2 += written;

    ExSetTimer(TimerHandle2, NOWAKE_TIMER_INTERVAL, 0, &TimerParam2);
}
void TestNoWakeTimer()
{
    TimerHandle2 = ExAllocateTimer(ExTimerCallback2, NULL, EX_TIMER_NO_WAKE);
    if (NULL != TimerHandle2)
    {
        ExInitializeSetTimerParameters(&TimerParam2);

        //indicates CPU should wakeup immediately even got timer in low power state.
        TimerParam2.NoWakeTolerance = NOWAKE_TIMER_TOLERANCE;   //8ms

        //set timer due time 500us.
        ExSetTimer(TimerHandle2, NOWAKE_TIMER_INTERVAL, 0, &TimerParam2);
    }

    LARGE_INTEGER interval = { 0 };
    interval.QuadPart = RUNTIME_IN_EACH_TEST;
    KeDelayExecutionThread(KernelMode, FALSE, &interval);

    if (NULL != TimerHandle2)
    {
        EXT_DELETE_PARAMETERS param = { 0 };
        ExInitializeDeleteTimerParameters(&param);
        ExDeleteTimer(TimerHandle2, TRUE, TRUE, &param);
    }
}
void PrintNoWakeTimerLog()
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[NoWake Timer Testing]\n");
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%s", LogData2);
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
    TestHighResTimer();
    TestNoWakeTimer();

    PrintHighResTimerLog();
    PrintNoWakeTimerLog();

    return STATUS_INTERNAL_ERROR;
}

EXTERN_C_END
