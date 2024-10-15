#include <ntddk.h>
#include <wdm.h>
#include <minwindef.h>

#include "ProcessFunctions.h"
#include "FileSignatureCheck.h"
#include "Utils.h"

QUERY_INFO_PROCESS ZwQueryInformationProcess;
//CI_FREE_POLICY_INFO CiFreePolicyInfo;
//CI_VALIDATE_FILE_OBJECT CiValidateFileObject;


EXTERN_C_START
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    BOOLEAN ok = FALSE;
    DbgBreakPoint();
    ok = LoadUndocumentKernelAPI((PVOID*)&ZwQueryInformationProcess, L"ZwQueryInformationProcess");
    if(!ok)
    {
        PrintKdMsg("[Roy] ZwQueryInformationProcess() not found!\n");
        return STATUS_UNSUCCESSFUL;
    }

    //If remark this line, "sc stop" command will be failed 
    //because "STOPPABLE" feature of driver need this callback.
    DriverObject->DriverUnload = DriverUnload;

    PrintKdMsg("[Roy] DriverEntry!\n");
    return PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCB, FALSE);
}

void DriverUnload(
    _In_ struct _DRIVER_OBJECT* DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCB, TRUE);
    PrintKdMsg("[Roy] DriverUnload!\n");
}
EXTERN_C_END
