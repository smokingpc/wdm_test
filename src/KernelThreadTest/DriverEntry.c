#include <wdm.h>

#define MYTAG 'YOR'

DRIVER_INITIALIZE   DriverEntry;
KSTART_ROUTINE      ThreadFunction;

static HANDLE ThreadHandle1 = NULL;
static HANDLE ThreadHandle2 = NULL;
static KEVENT EventDone1;
static KEVENT EventDone2;

static KEVENT* EventDone[2] = {&EventDone1, &EventDone2};

static KEVENT EventNotify1;
static KEVENT EventNotify2;

void ThreadFunction1(PVOID context)
{
    UNREFERENCED_PARAMETER(context);
    LARGE_INTEGER wait;
    wait.QuadPart = 0;
    int max_size = 1000000;
    LARGE_INTEGER freq;
    LARGE_INTEGER counter_begin;
    LARGE_INTEGER counter_end;

    int i=0;
    LARGE_INTEGER *counter = ExAllocatePoolWithTag(PagedPool, sizeof(LARGE_INTEGER)* max_size, MYTAG);
    for (int j = 0; j < max_size; j++)
        counter[j].QuadPart = 0;

    counter_begin = KeQueryPerformanceCounter(&freq);
    //NTSTATUS status = STATUS_TIMEOUT;
    while ((STATUS_TIMEOUT == KeWaitForSingleObject(&EventNotify1, Executive, KernelMode, FALSE, &wait)))// && i< max_size)
    {
        LARGE_INTEGER temp = KeQueryPerformanceCounter(&freq);
        if(i<max_size)
            counter[i++].QuadPart = temp.QuadPart;
    }
    counter_end = KeQueryPerformanceCounter(&freq);

    DbgBreakPoint();
    ExFreePoolWithTag(counter, MYTAG);
    KeSetEvent(EventDone[0], IO_NO_INCREMENT, FALSE);
}

void ThreadFunction2(PVOID context)
{
    UNREFERENCED_PARAMETER(context);
    //LARGE_INTEGER wait;
    //wait.QuadPart = -1000;
    //LARGE_INTEGER freq;
    //LARGE_INTEGER counter[1000];
    //LARGE_INTEGER counter_begin;
    //LARGE_INTEGER counter_end;

    //for(int j=0; j<1000; j++)
    //    counter[j].QuadPart=0;

    //int i = 0;
    
    //counter_begin = KeQueryPerformanceCounter(&freq);
    //while ((STATUS_TIMEOUT == KeWaitForSingleObject(&EventNotify2, Executive, KernelMode, FALSE, &wait)) && i < 1000)
    //{
    //    LARGE_INTEGER temp = KeQueryPerformanceCounter(&freq);
    //    counter[i++].QuadPart = temp.QuadPart;
    //}
    //counter_end = KeQueryPerformanceCounter(&freq);

    //DbgBreakPoint();
    KeSetEvent(EventDone[1], IO_NO_INCREMENT, FALSE);
}

NTSTATUS __stdcall
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    //NTSTATUS status = STATUS_SUCCESS;

    //LARGE_INTEGER freq;
    //LARGE_INTEGER counter_begin;
    //LARGE_INTEGER counter_end;

    //KeInitializeEvent(&EventDone, NotificationEvent, FALSE);
    KeInitializeEvent(&EventNotify1, NotificationEvent, FALSE);
    KeInitializeEvent(&EventNotify2, NotificationEvent, FALSE);
    KeInitializeEvent(EventDone[0], NotificationEvent, FALSE);
    KeInitializeEvent(EventDone[1], NotificationEvent, FALSE);

    PsCreateSystemThread(&ThreadHandle1, GENERIC_ALL, NULL, NULL, NULL, ThreadFunction1, NULL);
    PsCreateSystemThread(&ThreadHandle1, GENERIC_ALL, NULL, NULL, NULL, ThreadFunction2, NULL);
    
    //LARGE_INTEGER delay;
    //delay.QuadPart = -100000000;
    //counter_begin = KeQueryPerformanceCounter(&freq);
    //status = KeDelayExecutionThread(KernelMode, FALSE, &delay);
    //counter_end = KeQueryPerformanceCounter(&freq);
    //DbgBreakPoint();
    //    NTSTATUS status = KeWaitForSingleObject(&EventDone, Executive, KernelMode, TRUE, NULL);
    //KeSetEvent(&EventNotify1, IO_NO_INCREMENT, FALSE);
    //KeSetEvent(&EventNotify2, IO_NO_INCREMENT, FALSE);
    KeWaitForMultipleObjects(2, EventDone, WaitAll, Executive, KernelMode, FALSE, NULL, NULL);

    return STATUS_INTERNAL_ERROR;
}

