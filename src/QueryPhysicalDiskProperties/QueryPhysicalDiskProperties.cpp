// QueryPhysicalDiskProperties.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <tchar.h>
#include <windows.h>
#include <string>
#include "StorageQueryFunctions.h"

void Usage()
{
    printf("Usage: QueryPhysicalDiskProperties.exe [physical_drive_devpath]\n");
    printf("e.g.: QueryPhysicalDiskProperties.exe \\\\.\\PhysicalDrive1 \n=n");
}


//disk_name => "\\\\.\\PhysicalDrive1"
void QueryPhysicalDiskProperties(_TCHAR *disk_name)
{
    HANDLE disk = CreateFile(disk_name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if(INVALID_HANDLE_VALUE != disk)
    {
        QueryAdapterProperties(disk);
        QueryDeviceProperties(disk);
        QueryWriteCacheProperties(disk);
        QueryDiskGeometry(disk);
        QueryDiskCapacity(disk);
        CloseHandle(disk);
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    if (argc < 2)
    {
        Usage();
        return -1;
    }

    _TCHAR *phydisk = argv[1];
    QueryPhysicalDiskProperties(phydisk);
}
