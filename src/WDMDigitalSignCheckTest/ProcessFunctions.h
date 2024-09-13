#pragma once

#define     TAG_PROCNAME        ((ULONG)'MANP')
#define     PROCNAME_LEN        PAGE_SIZE

//Function Prototype for undocumented ZwQueryInformationProcess()
typedef NTSTATUS(*QUERY_INFO_PROCESS) (
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS GetProcessName(
    _In_ PEPROCESS proc, 
    _Inout_ PVOID buffer, 
    _In_ ULONG buf_size);
void ProcessNotifyCB(
    _Inout_            PEPROCESS proc,
    _In_               HANDLE pid,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO info);
