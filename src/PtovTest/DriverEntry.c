//#include <ntifs.h>
#include <wdm.h>

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
    KEVENT *EventDone;
    size_t buf_size;
    PVOID buffer;
    KEVENT* EventStop;
} MY_CTX, *PMY_CTX;

static MY_CTX* CreateContext(PVOID buffer, size_t size, KEVENT *notify, KEVENT* done)
{
    MY_CTX* ctx = ExAllocatePoolWithTag(NonPagedPool, sizeof(MY_CTX), MYTAG);
    ctx->EventStop = notify;
    ctx->EventDone = done;
    ctx->buffer = buffer;
    ctx->buf_size = size;

    return ctx;
}

static PFN_NUMBER GetFirstPfnFromVirtAddr(PVOID va, ULONG va_size)
{
    PMDL mdl = IoAllocateMdl(va, va_size, FALSE, FALSE, NULL);
    //ULONG page_count = ADDRESS_AND_SIZE_TO_SPAN_PAGES(va, va_size);
    PFN_NUMBER ret = 0;

    MmProbeAndLockPages(mdl, KernelMode, IoModifyAccess);
    MmBuildMdlForNonPagedPool(mdl);

    //content of PFN array is only valid when page is locked
    PPFN_NUMBER pfns = MmGetMdlPfnArray(mdl);
    ret = pfns[0];
    IoFreeMdl(mdl);

    return ret;
}

static PMDL CreateMdlByPfn(ULONG va_size, PFN_NUMBER pfn)
{
    CSHORT mdl_size = 0;
    ULONG page_count = 1;//ADDRESS_AND_SIZE_TO_SPAN_PAGES(ctx->buffer, ctx->buf_size);

    //referenced from WRK...
    //IOP_FIXED_SIZE_MDL_PFNS == 17
    if (page_count <= 17)
    {
        page_count = 17;
        //mdl_flag = MDL_ALLOCATED_FIXED_SIZE;
    }
    mdl_size = (CSHORT)(sizeof(MDL) + page_count * sizeof(PFN_NUMBER));
    PMDL mdl = ExAllocatePoolWithTag(NonPagedPool, mdl_size, MYTAG);

    RtlZeroMemory(mdl, mdl_size);
    MmInitializeMdl(mdl, 0, va_size);
    mdl->MdlFlags = MDL_ALLOCATED_FIXED_SIZE;

    PPFN_NUMBER array = MmGetMdlPfnArray(mdl);
    array[0] = pfn;

    return mdl;
}

void ThreadFunction(PVOID context)
{
    MY_CTX* ctx = (MY_CTX*) context;
    PFN_NUMBER pfn = GetFirstPfnFromVirtAddr(ctx->buffer, (ULONG)ctx->buf_size);
    PMDL mdl = CreateMdlByPfn((ULONG)ctx->buf_size, pfn);

    //cut a reserved address range for next mapping
    PVOID reserved_ptr = MmAllocateMappingAddress(ctx->buf_size, MYTAG);
    //map reserved address to physical pages (manipulate PXE/PDE/PTE tree and modify VAD...)
    PVOID mapped_ptr = MmMapLockedPagesWithReservedMapping(reserved_ptr, MYTAG, mdl, MmWriteCombined);
    //note : after mapping, the mapped real virtual address is in MDL::MappedSystemVa

    UNREFERENCED_PARAMETER(mapped_ptr);
    DbgBreakPoint();

    MmUnmapReservedMapping(mapped_ptr, MYTAG, mdl);
    MmFreeMappingAddress(reserved_ptr, MYTAG);
    ExFreePoolWithTag(mdl, MYTAG);

    KeSetEvent(ctx->EventDone, IO_NO_INCREMENT, FALSE);
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
    char *cursor = buffer;
    RtlFillMemory(cursor, 4, 'G');
    cursor += 4;
    RtlFillMemory(cursor, 4, 'R');
    cursor += 4;
    RtlFillMemory(cursor, 4, 'A');
    cursor += 4;
    RtlFillMemory(cursor, 4, 'I');
    cursor += 4;
    RtlFillMemory(cursor, 4, 'D');
    cursor += 4;
    DbgBreakPoint();

    KeInitializeEvent(&EventNotify, NotificationEvent, FALSE);
    KeInitializeEvent(&EventDone1, NotificationEvent, FALSE);
    MY_CTX *ctx = CreateContext(buffer, buf_size, &EventNotify, &EventDone1);
    status = PsCreateSystemThread(&ThreadHandle1, GENERIC_ALL, NULL, NULL, NULL, ThreadFunction, ctx);
    
    if(NT_SUCCESS(status))
    {
        ZwClose(ThreadHandle1);
        LARGE_INTEGER delay;
        delay.QuadPart = -100000000;
        //status = KeDelayExecutionThread(KernelMode, FALSE, &delay);
        KeSetEvent(&EventNotify, IO_NO_INCREMENT, FALSE);
        KeWaitForSingleObject(&EventDone1, Executive, KernelMode, FALSE, &delay);
    }

    DbgBreakPoint();

    ExFreePoolWithTag(ctx, MYTAG);
    ExFreePoolWithTag(buffer, MYTAG);

    return STATUS_INTERNAL_ERROR;
}

