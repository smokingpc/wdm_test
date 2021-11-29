#include <wdm.h>
#include <ntifs.h>

#define MYTAG 'YOR'

DRIVER_INITIALIZE   DriverEntry;
KSTART_ROUTINE      ThreadFunction;

static HANDLE ThreadHandle1 = NULL;
static HANDLE ThreadHandle2 = NULL;
static KEVENT EventDone1;
static KEVENT EventDone2;

static KEVENT* EventDone[2] = {&EventDone1, &EventDone2};

static KEVENT EventNotify;
//static KEVENT EventNotify2;

typedef struct _MY_CTX
{
    //KEVENT *EventDone;
    size_t buf_size;
    PVOID buffer;
    KEVENT* EventStop;
} MY_CTX, *PMY_CTX;

static MY_CTX* CreateContext(PVOID buffer, size_t size, KEVENT *notify)
{
    MY_CTX* ctx = ExAllocatePoolWithTag(NonPagedPool, sizeof(MY_CTX), MYTAG);
    ctx->EventStop = notify;
    ctx->buffer = buffer;
    ctx->buf_size = size;
}

void ThreadFunction(PVOID context)
{
    MY_CTX* ctx = (MY_CTX*) context;
    PVOID reserved_ptr =MmAllocateMappingAddress(ctx->buf_size, MYTAG);
    PMDL old_mdl = IoAllocateMdl(ctx->buffer, ctx->buf_size, FALSE, FALSE, NULL);
    PPFN_NUMBER old_pfns = MmGetMdlPfnArray(old_mdl);
    ULONG page_count = (ctx->buf_size + (PAGE_SIZE-1)) / PAGE_SIZE;
    size_t mdl_size = sizeof(MDL) + page_count * sizeof(PFN_NUMBER);
    PMDL new_mdl = ExAllocatePoolWithTag(NonPagedPool, mdl_size, MYTAG);
    RtlZeroMemory(new_mdl, mdl_size);
    PPFN_NUMBER new_pfns = MmGetMdlPfnArray(new_mdl);

    for(ULONG i=0;i<page_count;i++)
    {
        new_pfns[i] = old_pfns[i];
    }

    new_mdl->Process = IoGetCurrentProcess();
    new_mdl->Size = mdl_size;
    //MDL_MAPPED_TO_SYSTEM_VA;
    new_mdl->StartVa = PAGE_ALIGN(reserved_ptr);
    new_mdl->ByteOffset = BYTE_OFFSET(reserved_ptr);
    new_mdl->ByteCount = ctx->buf_size;

    DbgBreakPoint();

    MmProbeAndLockPages(new_mdl, KernelMode, IoModifyAccess);
    PVOID new_mapped_ptr = MmMapLockedPagesSpecifyCache(new_mdl, KernelMode, 
            MmWriteCombined, PAGE_ALIGN(reserved_ptr), FALSE, NormalPagePriority);

    DbgBreakPoint();
}

NTSTATUS __stdcall
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status = STATUS_SUCCESS;
    size_t buf_size = PAGE_SIZE;
    PVOID buffer = ExAllocatePoolWithTag(PagedPool, buf_size, MYTAG);
    KeInitializeEvent(&EventNotify, NotificationEvent, FALSE);
    MY_CTX *ctx = CreateContext(buffer, buf_size, &EventNotify);
    status = PsCreateSystemThread(&ThreadHandle1, GENERIC_ALL, NULL, NULL, NULL, ThreadFunction, ctx);
    
    if(NT_SUCCESS(status))
    {
        LARGE_INTEGER delay;
        delay.QuadPart = -100000000;
        status = KeDelayExecutionThread(KernelMode, FALSE, &delay);
        KeSetEvent(&EventNotify, IO_NO_INCREMENT, FALSE);
        //KeWaitForSingleObject(&EventDone1, Executive, KernelMode, FALSE, NULL);
        ZwWaitForSingleObject(ThreadHandle1, FALSE, NULL);
        ZwClose(ThreadHandle1);
    }

    ExFreePoolWithTag(ctx, MYTAG);
    ExFreePoolWithTag(buffer, MYTAG);

    return STATUS_INTERNAL_ERROR;
}

