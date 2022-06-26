#include "driver.h"
#include <evntrace.h>
#include "TestEventLog.h"

NTSTATUS TestEventLog2(WDFDEVICE device)
{
    DEVICE_OBJECT* devobj = WdfDeviceWdmGetDeviceObject(device);
    USHORT dump_size = 200; //DumpDataSize should be aligned to sizeof(ULONG)
    UCHAR size = (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + dump_size);
    IO_ERROR_LOG_PACKET* log = (IO_ERROR_LOG_PACKET*)IoAllocateErrorLogEntry(devobj, size);
    if (NULL != log)
    {
        log->MajorFunctionCode = 0xFF;  // (optional)
        log->RetryCount = 0xFF;
        log->DumpDataSize = sizeof(ULONG);
        log->ErrorCode = STATUS_INTERNAL_ERROR; //shown on message %1, all followed string are printed in %2 ~ %n
        log->EventCategory = 2;
        log->FinalStatus = STATUS_CRC_ERROR;    //real NT_STATUS code returned to OS or from API? (optional)
        log->IoControlCode = 0x12345678;        //current IOCTL code of this error? (optional)
        log->NumberOfStrings = 1;               //how many unicode strings following after dump data?
        log->SequenceNumber = 0x28825252;         //seq no. of current IRP (optional)
        log->StringOffset = sizeof(IO_ERROR_LOG_PACKET) + log->DumpDataSize;
        log->UniqueErrorValue = 0x23939889;     //unique error code defined by driver.  (optional)

        UCHAR* ptr = ((UCHAR*)log) + log->StringOffset;
        RtlCopyMemory(ptr, L"My Test EventLog", wcslen(L"My Test EventLog") * sizeof(WCHAR));
        IoWriteErrorLogEntry(log);
        //IoFreeErrorLogEntry(log); //log entry will be freed in IoWriteErrorLogEntry();
    }
    return STATUS_INTERRUPTED;
}

NTSTATUS TestEventLog1(PDRIVER_OBJECT  driver)
{
    USHORT dump_size = 200; //DumpDataSize should be aligned to sizeof(ULONG)
    UCHAR size = (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + dump_size);
    DbgBreakPoint();
    IO_ERROR_LOG_PACKET* log = (IO_ERROR_LOG_PACKET*)IoAllocateErrorLogEntry(driver, size);
    if (NULL != log)
    {
        log->MajorFunctionCode = 0xFF;  // (optional)
        log->RetryCount = 0xFF;
        log->DumpDataSize = sizeof(ULONG);
        log->ErrorCode = STATUS_INTERNAL_ERROR; //shown on message %1, all followed string are printed in %2 ~ %n
        log->EventCategory = 2;
        log->FinalStatus = STATUS_CRC_ERROR;    //real NT_STATUS code returned to OS or from API? (optional)
        log->IoControlCode = 0x12345678;        //current IOCTL code of this error? (optional)
        log->NumberOfStrings = 1;               //how many unicode strings following after dump data?
        log->SequenceNumber = 0x28825252;         //seq no. of current IRP (optional)
        log->StringOffset = sizeof(IO_ERROR_LOG_PACKET) + log->DumpDataSize;
        log->UniqueErrorValue = 0x23939889;     //unique error code defined by driver.  (optional)

        UCHAR* ptr = ((UCHAR*)log) + log->StringOffset;
        RtlCopyMemory(ptr, L"My Test EventLog", wcslen(L"My Test EventLog") * sizeof(WCHAR));
        IoWriteErrorLogEntry(log);
    }
    return STATUS_INTERRUPTED;
}

NTSTATUS TestEventLog3()
{

    return STATUS_INTERRUPTED;
}