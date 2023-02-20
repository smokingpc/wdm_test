#include "common.h"

PVOID hNotifyEvent = NULL;

NTSTATUS __stdcall DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    //UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    status = IoRegisterPlugPlayNotification(
                EventCategoryDeviceInterfaceChange,
                PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                (PVOID)&GUID_DEVINTERFACE_STORAGEPORT,
                DriverObject,
                NotifyCallback,
                NULL,
                &hNotifyEvent);
    if (!NT_SUCCESS(status))
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[PnpEvent Test] IoRegisterPlugPlayNotification() failed. Status=%08X\n", status);
    return status;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    if(NULL != hNotifyEvent)
    {
        IoUnregisterPlugPlayNotification(hNotifyEvent);
        hNotifyEvent = NULL;
    }
}

NTSTATUS NotifyCallback(
    _In_ PVOID NotificationStructure,
    _Inout_opt_ PVOID Context)
{
    //This callback function always be called at PASSIVE_LEVEL.
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT target = NULL;
    PFILE_OBJECT target_file = NULL;
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION notify = 
            (PDEVICE_INTERFACE_CHANGE_NOTIFICATION) NotificationStructure;

    UNREFERENCED_PARAMETER(Context);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "[PnpEvent Test] Got Notification!\n");
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "    Ver=%d, Size=%d\n", notify->Version, notify->Size);
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "    Event=%08X-%04X-%04X-%016llX\n", 
            notify->Event.Data1, notify->Event.Data2, 
            notify->Event.Data3, *((UINT64*)notify->Event.Data4));
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "    InterfaceClass=%08X-%04X-%04X-%016llX\n",
        notify->InterfaceClassGuid.Data1, notify->InterfaceClassGuid.Data2,
        notify->InterfaceClassGuid.Data3, *((UINT64*)notify->InterfaceClassGuid.Data4));
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "    SymbolicLinkName=%S\n", notify->SymbolicLinkName->Buffer);

    //IoGetDeviceObjectPointer() should be called at PASSIVE_LEVEL
    status = IoGetDeviceObjectPointer(
                notify->SymbolicLinkName,
                FILE_READ_DATA, 
                &target_file,
                &target);

    if(NT_SUCCESS(status))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "    =>Src DeviceObject of event == %p\n", target);
        ObDereferenceObject(target_file);
    }

    return STATUS_SUCCESS;
}
