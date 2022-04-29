// SetPhyDiskReadonlyTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <string>
#include <iostream>

void Usage()
{
    printf("Usage: SetPhyDiskReadonlyTest.exe [physical_drive_devpath] [on|off]\n");
    printf("e.g.: If you want to set physical disk 4 to readonly,\n");
    printf("      you should use command: SetPhyDiskReadonlyTest.exe \\\\.\\PhysicalDrive4 on\n\n");
    printf("e.g2.: If you want physical disk 4 to be read/write (unset readonly),\n");
    printf("      you should use command: SetPhyDiskReadonlyTest.exe \\\\.\\PhysicalDrive4 off\n\n");
}

void SetPhysicalDiskReadonlyFlag(char *devpath, bool is_readonly)
{
//devpath example:  "\\\\.\\PhysicalDrive2"
    HANDLE disk = CreateFileA(devpath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if(INVALID_HANDLE_VALUE != disk)
    {
        DWORD ret_size = 0;
        SET_DISK_ATTRIBUTES attr = {0};
        attr.Version = sizeof(SET_DISK_ATTRIBUTES);
        attr.Persist = TRUE;
        attr.Attributes = (is_readonly? 0x00000002 : 0);
        attr.AttributesMask = 0x00000002;

        BOOL ok = DeviceIoControl(disk,
            IOCTL_DISK_SET_DISK_ATTRIBUTES,
            &attr,
            sizeof(SET_DISK_ATTRIBUTES),
            NULL,
            0,
            &ret_size,
            NULL);

        if(!ok)
            printf("DeviceIoControl() failed, LastError=%d\n", GetLastError());

        ok = DeviceIoControl(disk,
            IOCTL_DISK_UPDATE_PROPERTIES,
            NULL,
            0,
            NULL,
            0,
            &ret_size,
            NULL);

        if (!ok)
            printf("DeviceIoControl() failed, LastError=%d\n", GetLastError());

        CloseHandle(disk);
    }
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        Usage();
        return -1;
    }

    char* devpath = argv[1];
    char* mode = argv[2];

    bool readonly = (_strnicmp(mode, "on", 2)==0);
    SetPhysicalDiskReadonlyFlag(devpath, readonly);
}
