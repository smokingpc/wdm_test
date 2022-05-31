// CleanAndInitDisk.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <tchar.h>
#include <windows.h>
#include <string>
#include <iostream>
#include "CommonDefines.h"

#if 0
//this action will be failed if already create a volume on this disk.
void CleanDisk(HANDLE disk)
{
    BOOL ok = FALSE;
    char* buffer = new char[SIZE_1MB];
    DWORD written = 0;
    RtlZeroMemory(buffer, SIZE_1MB);
    ok = WriteFile(disk, buffer, SIZE_1MB, &written, NULL);
    if (!ok)
        _tprintf(__T("Write 1st 1MB zero to disk failed. LastError=%d\n"), GetLastError());

    ok = WriteFile(disk, buffer, SIZE_1MB, &written, NULL);
    if (!ok)
        _tprintf(__T("Write 2nd 1MB zero to disk failed. LastError=%d\n"), GetLastError());

    ok = WriteFile(disk, buffer, SIZE_1MB, &written, NULL);
    if (!ok)
        _tprintf(__T("Write 3rd 1MB zero to disk failed. LastError=%d\n"), GetLastError());

    delete[] buffer;
}
#endif

//init disk will overwrite the disk layout and destroy old partitions.
//mounted volume will be unmount, and the volume will be removed.
void InitDisk(HANDLE disk, PARTITION_STYLE style)
{
    BOOL ok = FALSE;
    CREATE_DISK param;
    DWORD ret_size = 0;

    param.PartitionStyle = style;
    if(style == PARTITION_STYLE_GPT)
    {
        param.Gpt.MaxPartitionCount = 128;  //EFI defined this value is 128 in current stage....
        CoCreateGuid(&param.Gpt.DiskId);
    }
    else if (style == PARTITION_STYLE_MBR)
    {
        param.Mbr.Signature = GetTickCount();
    }

    ok = DeviceIoControl(
        disk,
        IOCTL_DISK_CREATE_DISK,
        &param,
        sizeof(CREATE_DISK),
        NULL,
        0,
        &ret_size,
        NULL);

    if(!ok)
        _tprintf(_T("IOCTL_DISK_CREATE_DISK failed. LastError=%d\n"), GetLastError());
}
void Usage()
{
    _tprintf(_T("======== WARNING ========\n"));
    _tprintf(_T("Please use this tool carefully...\n"));
    _tprintf(_T("======== WARNING ========\n"));
    _tprintf(_T("Usage: CleanAndInitDisk.exe [physical_drive_devpath] [gpt|mbr]\n"));
    _tprintf(_T("e.g.: If you want to init physical disk 3 to GPT type ,\n"));
    _tprintf(_T("      execute this tool as : CleanAndInitDisk.exe \\\\.\\PhysicalDrive3 gpt \n"));
}
BOOL ParseCmd(CMD_OPTION &option, int argc, TCHAR* argv[])
{
    if(argc <3)
        return FALSE;

    option.DiskDevName = argv[1];

    if(0 == _tcsnicmp(argv[2], _T("gpt"), 3))
        option.DiskStyle = PARTITION_STYLE_GPT;
    else if (0 == _tcsnicmp(argv[2], _T("mbr"), 3))
        option.DiskStyle = PARTITION_STYLE_MBR;
    else
        return FALSE;
    return TRUE;
}

int _tmain(int argc, TCHAR* argv[])
{
    CMD_OPTION option;
    if(FALSE == ParseCmd(option, argc, argv))
    {
        Usage();
        return -1;
    }
    _tprintf(_T("open disk %s\n"), option.DiskDevName.c_str());
    HANDLE disk = CreateFile(option.DiskDevName.c_str(), 
                            GENERIC_READ | GENERIC_WRITE, 
                            FILE_SHARE_READ | FILE_SHARE_WRITE, 
                            NULL, OPEN_EXISTING, 0, NULL);
    if(INVALID_HANDLE_VALUE != disk)
    {
        _tprintf(_T("do something\n"));
        //CleanDisk(disk);
        InitDisk(disk, option.DiskStyle);
        CloseHandle(disk);
    }
    else
        _tprintf(_T("Open disk [%s] failed. LastError=%d\n"), option.DiskDevName.c_str(), GetLastError());
    _tprintf(_T("End\n"));
}

