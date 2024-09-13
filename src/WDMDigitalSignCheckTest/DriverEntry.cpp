#include <ntddk.h>
#include <wdm.h>
#include <minwindef.h>

#include "ProcessFunctions.h"
#include "FileSignatureCheck.h"
#include "Utils.h"

QUERY_INFO_PROCESS ZwQueryInformationProcess;
CI_FREE_POLICY_INFO CiFreePolicyInfo;
CI_VALIDATE_FILE_OBJECT CiValidateFileObject;


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
    LoadUndocumentKernelAPI((PVOID*)&ZwQueryInformationProcess, L"ZwQueryInformationProcess");
    LoadUndocumentKernelAPI((PVOID*)&CiValidateFileObject, L"CiValidateFileObject");
    LoadUndocumentKernelAPI((PVOID*)&CiFreePolicyInfo, L"CiFreePolicyInfo");

    DriverObject->DriverUnload = DriverUnload;
    KdPrint(("DriverEntry!\n"));
    return PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCB, FALSE);
//    return STATUS_UNSUCCESSFUL;
}

void DriverUnload(
    _In_ struct _DRIVER_OBJECT* DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCB, TRUE);
    KdPrint(("DriverUnload!\n"));
}
EXTERN_C_END
