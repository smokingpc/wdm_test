#include <ntddk.h>
#include <wdm.h>
#include <wdmsec.h>
#include <minwindef.h>

#include "FileSignatureCheck.h"
#include "ProcessFunctions.h"
#include "Utils.h"

extern QUERY_INFO_PROCESS ZwQueryInformationProcess;
extern POBJECT_TYPE* PsProcessType;


//digest for testing. If want to test different signature, 
//modify this digest or you will always get failure.
static const BYTE TestSignDigest[] =
{
    0X00, 0x00, 0X00, 0x00, 0X00, 0x00, 0X00, 0x00
};

NTSTATUS GetProcessName(_In_ PEPROCESS proc, _Inout_ PVOID buffer, _In_ ULONG buf_size)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG ret_size = 0;
    HANDLE handle = nullptr;
    //PAGED_CODE(); // this eliminates the possibility of the IDLE Thread/Process

    if (proc == nullptr)
        return STATUS_INTERNAL_ERROR;

    status = RefObjectByPtr(
        proc, OBJ_KERNEL_HANDLE,
        nullptr, STANDARD_RIGHTS_READ | GENERIC_READ,
        *PsProcessType, KernelMode, &handle);

    if (!NT_SUCCESS(status))
    {
        PrintKdMsg("[Roy] ObOpenObjectByPointer Failed: %08x\n", status);
        goto end;
    }

    //string in buffer will be "full kernel path of image file".
    //e.g.  "\Device\HarddiskVolume20\Users\Administrator\Desktop\Roy\DirectIoTest.exe"
    status = ZwQueryInformationProcess(
        handle,
        ProcessImageFileName,
        buffer,  // entire UNICODE_STRING structure filled in result buffer.
        buf_size,  // buffer size
        &ret_size);

    if (!NT_SUCCESS(status))
    {
        PrintKdMsg("[Roy] failed to get process(%p) image name.\n", proc);
    }

end:
    if (handle != nullptr)
    {
        status = ZwClose(handle);
        handle = nullptr;
    }
    return status;
}

void ProcessNotifyCB(
    _Inout_            PEPROCESS proc,
    _In_               HANDLE pid,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO info)
{
    UNREFERENCED_PARAMETER(pid);
    //run at PASSIVE_LEVEL
    if (nullptr == info)
        return;     //process terminate

    UCHAR *buffer = (UCHAR*)ExAllocatePoolWithTag(PagedPool, PROCNAME_LEN, TAG_PROCNAME);
    if(nullptr == buffer)
        return;

    RtlZeroMemory(buffer, PROCNAME_LEN);
    NTSTATUS status = GetProcessName(proc, buffer, PROCNAME_LEN);
    if(NT_SUCCESS(status))
    {
        PUNICODE_STRING name = (PUNICODE_STRING)buffer;
        UNREFERENCED_PARAMETER(name);
        PrintKdMsg("[Roy] Launch process : %S\n", name->Buffer);
        PrintCertInfo(info->FileObject);
    }

    ExFreePoolWithTag(buffer, TAG_PROCNAME);
}
