
#include <wdm.h>
#include <ntstrsafe.h>

DRIVER_INITIALIZE   DriverEntry;
void TimerFunction(IN KDPC* dpc, PVOID context, PVOID arg1, PVOID arg2);


#define LOG_SIZE 8192//(PAGE_SIZE * 32);
//static char *TestLog = NULL;
static ULONG LogTag = 'GOLT';

#define TIMER_COUNT 5
#define INIT_WAIT 2500      //250 micro-seconds(2500 ticks)
#define REG_INTERVAL 1000    //register dpc in each interval
#define TEMP_STR_SIZE 128

typedef struct _TIMER_DPC_CTX
{
    char Log[LOG_SIZE];
    size_t LogSize;
    int MaxLogSize;
    volatile long IsRunning;
    volatile long StopFlag;

    //STRING TempStr;
    char TempStrBuf[TEMP_STR_SIZE+1];
    LARGE_INTEGER LastTimestamp;

    LARGE_INTEGER Interval;
    PKTIMER Timer;
    PKDPC   TimerDpc;
}TIMER_DPC_CTX, *PTIMER_DPC_CTX;

typedef struct _DPC_REG_CTX
{
    char Log[LOG_SIZE];
    char TempStrBuf[TEMP_STR_SIZE + 1];

    int TotalDPC;               //total timers to register
    int RegedDPC;               //current registered timers;
    //int CurrentCounter;         //how
    LARGE_INTEGER Interval;     //check interval
    //LARGE_INTEGER TimeLeft;     //how much time left to finish mission
    
    long IsRunning; 
    long StopFlag;

    PKTIMER Timers;
    PKDPC TimerDpcs;
    PTIMER_DPC_CTX TimerCtxs;
}DPC_REG_CTX, *PDPC_REG_CTX;


KTIMER Timer[TIMER_COUNT] = { 0 };
KDPC DpcReg = { 0 };
KDPC DpcTimer[TIMER_COUNT] = { 0 };
TIMER_DPC_CTX TimerCtx[TIMER_COUNT] = {0};
DPC_REG_CTX RegCtx = { 0 };

LARGE_INTEGER MinTimerInterval = { 0 };

__forceinline void StallDpcExecution(LARGE_INTEGER ticks)
{
    LARGE_INTEGER freq = { 0 };
    LARGE_INTEGER begin = KeQueryPerformanceCounter(&freq);

    while(1)
    {
        LARGE_INTEGER end = KeQueryPerformanceCounter(NULL);
        if(end.QuadPart - begin.QuadPart >= ticks.QuadPart)
            break;
    }
}

__forceinline void InitTimer(PKTIMER timer, PKDPC dpc, PTIMER_DPC_CTX ctx, PROCESSOR_NUMBER cpuid)
{
    ctx->Timer = timer;
    ctx->TimerDpc = dpc;
    ctx->Interval.QuadPart = MinTimerInterval.QuadPart;
    ctx->MaxLogSize = LOG_SIZE;

    //bind timer-dpc to cpu[0]
    KeInitializeDpc(dpc, TimerFunction, ctx);
    KeSetTargetProcessorDpcEx(dpc, &cpuid);
    KeInitializeTimer(timer);
}

__forceinline BOOLEAN DoLog(PTIMER_DPC_CTX ctx)
{
    LARGE_INTEGER freq = { 0 };
    LARGE_INTEGER timestamp = KeQueryPerformanceCounter(&freq);

    RtlZeroMemory(ctx->TempStrBuf, TEMP_STR_SIZE);

    RtlStringCbPrintfA(ctx->TempStrBuf, TEMP_STR_SIZE, "[PerformanceCounter] last(%lld) -> current(%lld), freq(%lld)\n",
            ctx->LastTimestamp.QuadPart, timestamp.QuadPart, freq.QuadPart);
    ctx->LastTimestamp = timestamp;
    size_t size = 0;
    RtlStringCbLengthA(ctx->TempStrBuf, TEMP_STR_SIZE, &size);
    if(ctx->LogSize + size < (ctx->MaxLogSize-1))
    {
        ctx->LogSize += size;
        RtlStringCbCatA(ctx->Log, LOG_SIZE, ctx->TempStrBuf);
        return TRUE;
    }

    return FALSE;
}

void TimerFunction(IN KDPC* dpc, PVOID context, PVOID arg1, PVOID arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(arg1);
    UNREFERENCED_PARAMETER(arg2);

    PTIMER_DPC_CTX ctx = (PTIMER_DPC_CTX)context;
    if(0 == ctx->StopFlag && DoLog(ctx) == TRUE)
    {
        KeSetTimer(ctx->Timer, ctx->Interval, ctx->TimerDpc);
    }
    else
    {
        KeCancelTimer(ctx->Timer);
        InterlockedDecrement(&ctx->IsRunning);
    }
}

void RegFunction(IN KDPC *dpc, PVOID context, PVOID arg1, PVOID arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(arg1);
    UNREFERENCED_PARAMETER(arg2);

    PDPC_REG_CTX ctx = (PDPC_REG_CTX)context;
    if(ctx->RegedDPC >= ctx->TotalDPC)
    {
        ctx->IsRunning = 0;
        return;
    }

    StallDpcExecution(ctx->Interval);
    LARGE_INTEGER freq = { 0 };
    LARGE_INTEGER timestamp = KeQueryPerformanceCounter(&freq);
    RtlZeroMemory(ctx->TempStrBuf, TEMP_STR_SIZE);
    RtlStringCbPrintfA(ctx->TempStrBuf, TEMP_STR_SIZE, "[RegCtx] current(%lld), freq(%lld)\n",
        timestamp.QuadPart, freq.QuadPart);
    RtlStringCbCatA(ctx->Log, LOG_SIZE, ctx->TempStrBuf);

    PKTIMER timer = &ctx->Timers[ctx->RegedDPC];
    PKDPC timer_dpc = &ctx->TimerDpcs[ctx->RegedDPC];
    PTIMER_DPC_CTX timer_ctx = &ctx->TimerCtxs[ctx->RegedDPC];

    InterlockedIncrement(&timer_ctx->IsRunning);
    LARGE_INTEGER due_time = timer_ctx->Interval;
    due_time.QuadPart -= 10*1000*50; //10ms + interval  , relative time.
    KeSetTimer(timer, due_time, timer_dpc);

    ctx->RegedDPC++;
    if (ctx->RegedDPC < ctx->TotalDPC)
        KeInsertQueueDpc(dpc, ctx, NULL);
    else
        ctx->IsRunning = 0;
}

void InitDPC()
{
    PROCESSOR_NUMBER cpuid;
    KeGetProcessorNumberFromIndex(0, &cpuid);

    ULONG maxt, mint, curt;
    ExQueryTimerResolution(&maxt, &mint, &curt);
    MinTimerInterval.QuadPart = -((LONGLONG)ExSetTimerResolution(mint, TRUE));
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0x0000FF00, "[TimerResolution]==> max(%d), min(%d), current(%d)\n", maxt, mint, curt);

//prepare timers, timer dpcs, and their context
    for(int i=0; i<TIMER_COUNT; i++)
    {
        PKTIMER timer = &Timer[i];
        PKDPC dpc = &DpcTimer[i];
        PTIMER_DPC_CTX ctx = &TimerCtx[i];
        InitTimer(timer, dpc, ctx, cpuid);
    }

//start DPC-Register routine
    {
        RegCtx.TotalDPC = TIMER_COUNT;
        RegCtx.Interval.QuadPart = REG_INTERVAL;
        //RegCtx.TimeLeft.QuadPart = RegCtx.Interval.QuadPart * RegCtx.TotalDPC;
        RegCtx.Timers = Timer;
        RegCtx.TimerDpcs = DpcTimer;
        RegCtx.TimerCtxs = TimerCtx;

        RegCtx.IsRunning = 1;
        //bind register-dpc to cpu[0]
        KeInitializeDpc(&DpcReg, RegFunction, &RegCtx);
        KeSetTargetProcessorDpcEx(&DpcReg, &cpuid);

        KeInsertQueueDpc(&DpcReg, &RegCtx, NULL);
    }
}

void DeInitDPC()
{
    InterlockedIncrement(&RegCtx.StopFlag);
    for (int i = 0; i < TIMER_COUNT; i++)
    {
        //KeCancelTimer(&Timer[i]);
        PTIMER_DPC_CTX ctx = &TimerCtx[i];
        InterlockedIncrement(&ctx->StopFlag);
    }

    ULONG maxt, mint, curt;
    ExQueryTimerResolution(&maxt, &mint, &curt);

    //wait all timer stopped to prevent BSoD
    int running = TIMER_COUNT;
    while(running > 0 || RegCtx.IsRunning > 0)
    {
        running = TIMER_COUNT;
        for (int i = 0; i < TIMER_COUNT; i++)
        {
            PTIMER_DPC_CTX ctx = &TimerCtx[i];
            if (0 == ctx->IsRunning)
                running--;
        }

        LARGE_INTEGER wait = { 0 };
        wait.QuadPart = maxt;

        KeDelayExecutionThread(KernelMode, FALSE, &wait);
    }
    
    //dump debug log. I just want to know the DPC interval in this test....
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0x0000FF00, RegCtx.Log);
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0x0000FF00, "\n\n");

    for (int i = 0; i < TIMER_COUNT; i++)
    {
        PTIMER_DPC_CTX ctx = &TimerCtx[i];

        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0x0000FF00, "[Timer-%d]==>\n", i);
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0x0000FF00, ctx->Log);

        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0x0000FF00, "\n\n");
    }
    
    for (int i = 0; i < TIMER_COUNT; i++)
    {
        KeCancelTimer(&Timer[i]);
    }
}

NTSTATUS __stdcall
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    InitDPC();
    LARGE_INTEGER wait = {0};
    wait.QuadPart = -10*1000*1000*20;
    KeDelayExecutionThread(KernelMode, FALSE, &wait);
    
    DeInitDPC();
    
    return STATUS_UNSUCCESSFUL;
}

