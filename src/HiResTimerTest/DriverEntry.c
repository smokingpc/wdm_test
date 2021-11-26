#include <wdm.h>
#include <ntstrsafe.h>
#define DEVPATH L"\\Device\\0000006b"
#define POOLTAG 'YOR'

KEVENT IoEvent;

NTSTATUS MyIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "MyIoCompletion IRP status(%d)\n", Irp->IoStatus.Status);

    IoFreeMdl(Irp->MdlAddress);
    Irp->MdlAddress = NULL;
    IoFreeIrp(Irp);

    KeSetEvent(&IoEvent, IO_NO_INCREMENT, FALSE);

    //if free IRP and MDL here, we MUST return STATUS_MORE_PROCESSING_REQUIRED to 
    //tell I/O manager:don't touch this IRP again....
    return STATUS_MORE_PROCESSING_REQUIRED;
}

void PrintBufferContent(PVOID buffer, ULONG size)
{
    unsigned char* cursor = buffer;
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[Content of Buffer]\n");
    for (ULONG i = 0; i < size; i = i + 16)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%p :\t %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
            cursor + i,
            cursor[i + 0], cursor[i + 1], cursor[i + 2], cursor[i + 3], cursor[i + 4], cursor[i + 5], cursor[i + 6], cursor[i + 7],
            cursor[i + 8], cursor[i + 9], cursor[i + 10], cursor[i + 11], cursor[i + 12], cursor[i + 13], cursor[i + 14], cursor[i + 15]);
    }
}

void DoTest()
{
    UNICODE_STRING objname = { 0 };
    PDEVICE_OBJECT pDevObj = NULL;
    PFILE_OBJECT pFileObj = NULL;
    PIRP pIrp = NULL;
    PMDL mdl = NULL;
    PVOID buffer = NULL;
    ULONG size = PAGE_SIZE / 4;
    PIO_STACK_LOCATION stack = NULL;

    KeInitializeEvent(&IoEvent, NotificationEvent, FALSE);
    RtlInitUnicodeString(&objname, DEVPATH);

    NTSTATUS status = IoGetDeviceObjectPointer(&objname, FILE_ALL_ACCESS, &pFileObj, &pDevObj);

    if (NT_SUCCESS(status))
    {
        pIrp = IoAllocateIrp(pDevObj->StackSize, FALSE);
        buffer = ExAllocatePoolWithTag(NonPagedPool, size, POOLTAG);
        mdl = IoAllocateMdl(buffer, size, FALSE, FALSE, pIrp);
        MmBuildMdlForNonPagedPool(mdl);

        RtlZeroMemory(buffer, size);

        stack = IoGetNextIrpStackLocation(pIrp);
        stack->DeviceObject = pDevObj;
        stack->FileObject = pFileObj;
        stack->MajorFunction = IRP_MJ_READ;
        stack->Parameters.Read.ByteOffset.QuadPart = 0;
        stack->Parameters.Read.Length = size;

        IoSetCompletionRoutine(pIrp, MyIoCompletion, NULL, TRUE, TRUE, TRUE);

        status = IoCallDriver(pDevObj, pIrp);
        if (status == STATUS_PENDING)
        {
            LARGE_INTEGER wait = { .QuadPart = 150000000 };
            status = KeWaitForSingleObject(&IoEvent, Executive, KernelMode, FALSE, &wait);
        }

        PrintBufferContent(buffer, size);

        if (NULL != buffer)
            ExFreePoolWithTag(buffer, POOLTAG);
    }
}


NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    DoTest();

    return STATUS_INTERNAL_ERROR;
}
