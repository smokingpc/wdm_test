#include <wdm.h>

KDPC DpcTest;
bool FlagStop = false;

void TestFunction(
    PKDPC dpc,
    PVOID ctx,
    PVOID sysarg1,
    PVOID sysarg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(ctx);
    UNREFERENCED_PARAMETER(sysarg1);
    UNREFERENCED_PARAMETER(sysarg2);

    while (!FlagStop)
    {
        KeStallExecutionProcessor(500);
    }
}

void TestThread(PVOID arg)
{
    BOOLEAN ok = FALSE;
    ULONG cpu = PtrToUlong(arg);
    GROUP_AFFINITY affinity = { 0 };
    affinity.Group = 0;
    affinity.Mask = 1ULL << cpu;
    //bind current thread to CPU-0
    KeSetSystemGroupAffinityThread(&affinity, NULL);
    ok = KeInsertQueueDpc(&DpcTest, NULL, NULL);

    while (!FlagStop)
    {
        LARGE_INTEGER interval = { 0 };
        interval.QuadPart = -10 * 1000 * 1000;
        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    }
}

EXTERN_C_START
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    KIRQL old_irql = PASSIVE_LEVEL;
    BOOLEAN ok = FALSE;

    DriverObject->DriverUnload = DriverUnload;

    GROUP_AFFINITY affinity = { 0 };
    affinity.Group = 0;
    affinity.Mask = 1ULL;
    //bind current thread to CPU-0
    KeSetSystemGroupAffinityThread(&affinity, NULL);
    KeInitializeDpc(&DpcTest, TestFunction, NULL);

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PROCESSOR_NUMBER proc = { 0 };

//Test1 : Try change TargetProcessor of DPC in each InsertQueueDpc
//result : by checking KDPC content, KeSetTargetProcessorDpcEx() seems 
//         not working if DPC is queued.
    KeRaiseIrql(DISPATCH_LEVEL, &old_irql);

    DbgBreakPoint();
    proc.Number = (UCHAR)1;
    status = KeSetTargetProcessorDpcEx(&DpcTest, &proc);
    ok = KeInsertQueueDpc(&DpcTest, NULL, NULL);
    KeStallExecutionProcessor(200);

    proc.Number = (UCHAR)2;
    status = KeSetTargetProcessorDpcEx(&DpcTest, &proc);
    ok = KeInsertQueueDpc(&DpcTest, NULL, NULL);
    KeStallExecutionProcessor(200);

    proc.Number = (UCHAR)3;
    status = KeSetTargetProcessorDpcEx(&DpcTest, &proc);
    ok = KeInsertQueueDpc(&DpcTest, NULL, NULL);
    KeStallExecutionProcessor(200);

    proc.Number = (UCHAR)1;
    status = KeSetTargetProcessorDpcEx(&DpcTest, &proc);
    ok = KeInsertQueueDpc(&DpcTest, NULL, NULL);
    KeStallExecutionProcessor(200);

    proc.Number = (UCHAR)2;
    status = KeSetTargetProcessorDpcEx(&DpcTest, &proc);
    ok = KeInsertQueueDpc(&DpcTest, NULL, NULL);
    KeStallExecutionProcessor(200);

    proc.Number = (UCHAR)3;
    status = KeSetTargetProcessorDpcEx(&DpcTest, &proc);
    ok = KeInsertQueueDpc(&DpcTest, NULL, NULL);
    KeStallExecutionProcessor(200);

    DbgBreakPoint();
    KeLowerIrql(PASSIVE_LEVEL);

    FlagStop = 1;
    KeStallExecutionProcessor(500);
    FlagStop = 0;

    //Test 2 : fire DPC in each thread
    //result : 2 DPC both running after InsertQueueDpc
    HANDLE thread1 = NULL, thread2 = NULL;
    KeInitializeDpc(&DpcTest, TestFunction, NULL);

    PsCreateSystemThread(
        &thread1, THREAD_ALL_ACCESS, NULL,
        NtCurrentProcess(), NULL, TestThread, ULongToPtr(1));
    PsCreateSystemThread(
        &thread2, THREAD_ALL_ACCESS, NULL,
        NtCurrentProcess(), NULL, TestThread, ULongToPtr(2));

    LARGE_INTEGER interval = { 0 };
    interval.QuadPart = -10 * 1000 * 1000;

    KeDelayExecutionThread(KernelMode, FALSE, &interval);
    DbgBreakPoint();
    while (!FlagStop)
    {
        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    }

    KeDelayExecutionThread(KernelMode, FALSE, &interval);

    return STATUS_UNSUCCESSFUL;
}

void DriverUnload(
    _In_ struct _DRIVER_OBJECT* DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    KdPrint(("DriverUnload!\n"));
}
EXTERN_C_END