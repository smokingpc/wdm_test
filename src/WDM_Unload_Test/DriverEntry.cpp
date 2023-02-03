#include <wdm.h>

EXTERN_C_START
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    //If remark this line, "sc stop" command will be failed 
    //because "STOPPABLE" feature of driver need this callback.
    //DriverObject->DriverUnload = DriverUnload;

    KdPrint(("DriverEntry!\n"));
    return STATUS_SUCCESS;
}

void DriverUnload(
    _In_ struct _DRIVER_OBJECT* DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    KdPrint(("DriverUnload!\n"));
}
EXTERN_C_END