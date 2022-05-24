#include <tchar.h>
#include <windows.h>
#include <string>
#include "StorageQueryFunctions.h"

#define SIZE_1MB    1048576
#define BUFFER_SIZE 4096


__inline BOOL DeviceIoStandardStorageQuery(HANDLE disk, STORAGE_PROPERTY_ID propid,
    PVOID in_buf, DWORD in_buf_size, PVOID out_buf, DWORD out_buf_size, DWORD* ret_size)
{
    RtlZeroMemory(in_buf, in_buf_size);
    RtlZeroMemory(out_buf, out_buf_size);

    STORAGE_PROPERTY_QUERY* query = (STORAGE_PROPERTY_QUERY*)in_buf;
    query->QueryType = PropertyStandardQuery;
    query->PropertyId = propid;

    BOOL ok = DeviceIoControl(
        disk,
        IOCTL_STORAGE_QUERY_PROPERTY,
        in_buf,
        in_buf_size,
        out_buf,
        out_buf_size,
        ret_size,
        NULL);
    if (!ok)
        _tprintf(_T("DeviceIoControl failed, LastError=0x%08X\n"), GetLastError());

    return ok;
}

__inline BOOL DeviceIoQuery(HANDLE disk,DWORD ioctl_code , PVOID in_buf, DWORD in_buf_size, 
                            PVOID out_buf, DWORD out_buf_size, DWORD* ret_size)
{
    BOOL ok = DeviceIoControl(
        disk,
        ioctl_code,
        in_buf,
        in_buf_size,
        out_buf,
        out_buf_size,
        ret_size,
        NULL);
    if (!ok)
        _tprintf(_T("DeviceIoControl failed, LastError=0x%08X\n"), GetLastError());

    return ok;
}

void QueryDeviceProperties(HANDLE disk)
{
    UCHAR in_buffer[BUFFER_SIZE] = { 0 };
    UCHAR out_buffer[BUFFER_SIZE] = { 0 };
    DWORD ret_size = 0;

    BOOL ok = DeviceIoStandardStorageQuery(disk, StorageDeviceProperty, in_buffer, _countof(in_buffer),
        out_buffer, _countof(out_buffer), &ret_size);

    if(!ok)
    {
        _tprintf(_T("QueryDeviceProperties failed....\n\n"));
        return;
    }

    STORAGE_DEVICE_DESCRIPTOR* result = (STORAGE_DEVICE_DESCRIPTOR*)out_buffer;
    _tprintf(_T("[StorageDeviceProperty]\n"));
    _tprintf(_T("Version=%d, Size=%d\n"), result->Version, result->Size);
    _tprintf(_T("DeviceType=%d, DeviceTypeModifier=%d, RemovableMedia=%d, CommandQueueing=%d\n"),
        result->DeviceType, result->DeviceTypeModifier, result->RemovableMedia, result->CommandQueueing);
    _tprintf(_T("VendorIdOffset=%d, ProductIdOffset=%d, ProductRevisionOffset=%d, SerialNumberOffset=%d\n"),
        result->VendorIdOffset, result->ProductIdOffset, result->ProductRevisionOffset, result->SerialNumberOffset);

    _tprintf(_T("BusType=%d, RawPropertiesLength=%d\n"), result->BusType, result->RawPropertiesLength);
    
    for(DWORD i=0; i< result->RawPropertiesLength; i++)
    {
        _tprintf(_T("%02X "),result->RawDeviceProperties[i]);
        if(i!=0 && i%16==0)
            _tprintf(_T("\n"));
    }
    _tprintf(_T("\n\n"));
}
void QueryAdapterProperties(HANDLE disk)
{
    UCHAR in_buffer[BUFFER_SIZE] = { 0 };
    UCHAR out_buffer[BUFFER_SIZE] = { 0 };
    DWORD ret_size = 0;

    BOOL ok = DeviceIoStandardStorageQuery(disk, StorageAdapterProperty, in_buffer, _countof(in_buffer),
        out_buffer, _countof(out_buffer), &ret_size);
    if (!ok)
    {
        _tprintf(_T("QueryAdapterProperties failed....\n\n"));
        return;
    }

    STORAGE_ADAPTER_DESCRIPTOR* result = (STORAGE_ADAPTER_DESCRIPTOR*)out_buffer;
    _tprintf(_T("[StorageAdapterProperty]\n"));
    _tprintf(_T("Version=%d, Size=%d\n"), result->Version, result->Size);
    _tprintf(_T("MaximumTransferLength=%d, MaximumPhysicalPages=%d, AlignmentMask=%d\n"),
        result->MaximumTransferLength, result->MaximumPhysicalPages, result->AlignmentMask);
    _tprintf(_T("AdapterUsesPio=%d, AdapterScansDown=%d, CommandQueueing=%d, AcceleratedTransfer=%d\n"),
        result->AdapterUsesPio, result->AdapterScansDown, result->CommandQueueing, result->AcceleratedTransfer);
    _tprintf(_T("BusType=%d, BusMajorVersion=%d,BusMinorVersion=%d\n"), result->BusType, result->BusMajorVersion, result->BusMinorVersion);
    _tprintf(_T("SrbType=%d, AddressType=%d\n\n"), result->SrbType, result->AddressType);
}
void QueryWriteCacheProperties(HANDLE disk)
{
    UCHAR in_buffer[BUFFER_SIZE] = { 0 };
    UCHAR out_buffer[BUFFER_SIZE] = { 0 };
    DWORD ret_size = 0;

    BOOL ok = DeviceIoStandardStorageQuery(disk, StorageDeviceWriteCacheProperty, in_buffer, _countof(in_buffer),
        out_buffer, _countof(out_buffer), &ret_size);
    if (!ok)
    {
        _tprintf(_T("QueryWriteCacheProperties failed....\n\n"));
        return;
    }

    STORAGE_WRITE_CACHE_PROPERTY* result = (STORAGE_WRITE_CACHE_PROPERTY*)out_buffer;
    _tprintf(_T("[StorageDeviceWriteCacheProperty]\n"));
    _tprintf(_T("Version=%d, Size=%d\n"), result->Version, result->Size);
    _tprintf(_T("WriteCacheType=%d, WriteCacheEnabled=%d, WriteCacheChangeable=%d, WriteThroughSupported=%d\n"), 
        result->WriteCacheType, result->WriteCacheEnabled, result->WriteCacheChangeable, result->WriteThroughSupported);
    _tprintf(_T("FlushCacheSupported=%d, UserDefinedPowerProtection=%d, NVCacheEnabled=%d\n\n"),
        result->FlushCacheSupported, result->UserDefinedPowerProtection, result->NVCacheEnabled);
}
void QueryDiskGeometry(HANDLE disk)
{
//this is old way to query disk size...
//now we use IOCTL_STORAGE_READ_CAPACITY to fit storport system.
    UCHAR out_buffer[BUFFER_SIZE] = { 0 };
    DWORD ret_size = 0;

    BOOL ok = DeviceIoQuery(disk, IOCTL_DISK_GET_DRIVE_GEOMETRY , 
                    NULL, 0, out_buffer, _countof(out_buffer), &ret_size);
    if (!ok)
    {
        _tprintf(_T("QueryDiskGeometry failed....\n"));
        return;
    }

    DISK_GEOMETRY_EX *result = (DISK_GEOMETRY_EX*)out_buffer;
    _tprintf(_T("[QueryDiskGeometry]\n"));
    //Geometry field can be "faked" by driver.... don't trust it much....
    _tprintf(_T("DiskSize=%lld(Bytes), MediaType=%d\n"), 
            result->DiskSize.QuadPart, result->Geometry.MediaType);
    _tprintf(_T("Cylinders=%lld, TracksPerCylinder=%d, SectorsPerTrack=%d, BytesPerSector=%d\n\n"),
        result->Geometry.Cylinders.QuadPart, result->Geometry.TracksPerCylinder,
        result->Geometry.SectorsPerTrack, result->Geometry.BytesPerSector);
}

void QueryDiskCapacity(HANDLE disk)
{
    UCHAR out_buffer[BUFFER_SIZE] = { 0 };
    DWORD ret_size = 0;

    BOOL ok = DeviceIoQuery(disk, IOCTL_STORAGE_READ_CAPACITY,
        NULL, 0, out_buffer, _countof(out_buffer), &ret_size);
    if (!ok)
    {
        _tprintf(_T("QueryDiskCapacity failed....\n"));
        return;
    }

    STORAGE_READ_CAPACITY* result = (STORAGE_READ_CAPACITY*)out_buffer;
    _tprintf(_T("[QueryDiskGeometry]\n"));
    _tprintf(_T("Version=%d, Size=%d\n"), result->Version, result->Size);
    _tprintf(_T("BlockLength=%d, NumberOfBlocks=%lld, DiskLength=%lld\n\n"),
        result->BlockLength, result->NumberOfBlocks.QuadPart, result->DiskLength);
}
