#include "TimerCostTest.h"

KDPC TimerDPC;
PKDPC DummyDPCs = NULL;
ULONG CpuCount = 0;
LARGE_INTEGER TimerInterval = { 0 };

void DummyDpcRoutine(
    _In_ struct _KDPC* dpc,
    _In_opt_ PVOID ctx,
    _In_opt_ PVOID arg1,
    _In_opt_ PVOID arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(ctx);
    UNREFERENCED_PARAMETER(arg1);
    UNREFERENCED_PARAMETER(arg2);
}
void TimerDpcRoutine(
    _In_ struct _KDPC* dpc,
    _In_opt_ PVOID ctx,
    _In_opt_ PVOID arg1,
    _In_opt_ PVOID arg2)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(ctx);
    UNREFERENCED_PARAMETER(arg1);
    UNREFERENCED_PARAMETER(arg2);
    KeSetTimer(&MyTimer, TimerInterval, dpc);
}

NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //If remark this line, "sc stop" command will be failed 
    //because "STOPPABLE" feature of driver need this callback.
    DriverObject->DriverUnload = DriverUnload;

    KeInitializeTimer(&MyTimer);
    KeInitializeDpc(&TimerDPC, TimerDpcRoutine, NULL);
    CpuCount = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
    DummyDPCs = ExAllocatePoolWithTag(NonPagedPool, CpuCount*sizeof(KDPC), TAG_DPC_ARRAY);
    if(NULL == DummyDPCs)
        return STATUS_MEMORY_NOT_ALLOCATED;
    RtlZeroMemory(DummyDPCs, CpuCount * sizeof(KDPC));

    for(ULONG i=0; i<CpuCount; i++)
    {
        PROCESSOR_NUMBER cpu_num = { 0 };
        status = KeGetProcessorNumberFromIndex(i, &cpu_num);
        if (!NT_SUCCESS(status)) 
        {
            DbgPrint("KeGetProcessorNumberFromIndex failed\n");
            return status;
        }

        KeInitializeDpc(&DummyDPCs[i], DummyDpcRoutine, NULL);
        KeSetTargetProcessorDpcEx(&DummyDPCs[i], &cpu_num);
    }

    ULONG max_res = 0, min_res = 0, curr_res = 0, new_res = 0;
    ExQueryTimerResolution(&max_res, &min_res, &curr_res);
    if(min_res < 5000)
        min_res = 5000;
    new_res = ExSetTimerResolution(min_res, TRUE);
    TimerInterval.QuadPart = (-1 * (LONGLONG)new_res); //poll interval to 500 us.

    //KeSetTimer(&MyTimer, TimerInterval, &TimerDPC);

    DbgPrint("===> DriverEntry!\n");
    return STATUS_SUCCESS;
}

void DriverUnload(
    _In_ struct _DRIVER_OBJECT* DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    KeCancelTimer(&MyTimer);
    KeRemoveQueueDpc(&TimerDPC);

    ExSetTimerResolution(0, FALSE);

    if(NULL != DummyDPCs)
    {
        for (ULONG i = 0; i < CpuCount; i++)
        {
            KeRemoveQueueDpc(&DummyDPCs[i]);
        }

        LARGE_INTEGER wait = {0};
        wait.QuadPart = -1*1000*1000*10;
        KeDelayExecutionThread(KernelMode, FALSE, &wait);
        ExFreePoolWithTag(DummyDPCs, TAG_DPC_ARRAY);
    }

    DbgPrint("<=== DriverUnload!\n");
}
