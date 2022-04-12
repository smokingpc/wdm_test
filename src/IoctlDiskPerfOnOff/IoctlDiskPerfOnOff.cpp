// IoctlDiskPerfOnOff.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <Setupapi.h>
#include <Ntddstor.h>
#include <stdio.h>
#include <string>
#include <list>
#pragma comment( lib, "setupapi.lib" )

#define PHYSICAL_DISK_FORMAT    L"\\\\?\\PhysicalDrive%d"


typedef struct _PHYDISK_INFO
{
public:
    std::wstring PhydiskName = L"";     //e.g. \\?\PhysicalDrive3
    std::wstring DevPath = L"";         //DeviceName of disk, e.g.  \\?\pci#ven_1987&dev_5013&subsys_50131987&rev_01#4&31783dac&0&00c1#{2accfe60-c130-11d2-b082-00a0c91efb8b}
}PHYDISK_INFO, *PPHYDISK_INFO;


void EnumPhysicalDisks(std::list<PHYDISK_INFO> &result)
{
    HDEVINFO devinfo = NULL;
    GUID disk_class_guid = GUID_DEVINTERFACE_DISK;

    devinfo = SetupDiGetClassDevs(&disk_class_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (INVALID_HANDLE_VALUE != devinfo)
    {
        SP_DEVICE_INTERFACE_DATA ifdata = { 0 };
        ifdata.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        DWORD devid = 0;
        while (TRUE == SetupDiEnumDeviceInterfaces(devinfo, NULL, &disk_class_guid, devid, &ifdata))
        {
            DWORD need_size = 0;
            DWORD return_size = 0;
            BOOL ok = FALSE;
            PSP_DEVICE_INTERFACE_DETAIL_DATA ifdetail = NULL;
            devid++;
            SetupDiGetDeviceInterfaceDetail(devinfo, &ifdata, NULL, 0, &need_size, NULL);
            need_size = need_size * 2;
            BYTE* buffer = new BYTE[need_size];
            ZeroMemory(buffer, need_size);

            ifdetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buffer;
            ifdetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            ok = SetupDiGetDeviceInterfaceDetail(devinfo, &ifdata, ifdetail, need_size, &need_size, NULL);
            if (TRUE == ok)
            {
                HANDLE device = CreateFile(ifdetail->DevicePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, NULL);
                if (INVALID_HANDLE_VALUE != device)
                {
                    STORAGE_DEVICE_NUMBER disk_number = { 0 };
                    return_size = 0;
                    ok = DeviceIoControl(device,
                        IOCTL_STORAGE_GET_DEVICE_NUMBER,
                        NULL,
                        0,
                        &disk_number,
                        sizeof(STORAGE_DEVICE_NUMBER),
                        &return_size,
                        NULL);

                    if (TRUE == ok)
                    {
                        PHYDISK_INFO diskinfo;
                        WCHAR phyname[128] = {0};
                        swprintf(phyname, 128, PHYSICAL_DISK_FORMAT, disk_number.DeviceNumber);
                        diskinfo.PhydiskName = phyname;
                        diskinfo.DevPath = ifdetail->DevicePath;
                        result.push_back(diskinfo);
                    }
                    CloseHandle(device);
                }
            }
            delete[] buffer;
        }
    }
}

void PrintDiskList(std::list<PHYDISK_INFO> &disklist)
{
    wprintf(L"[Print found disklist]\n");
    for(auto item : disklist)
    {
        wprintf(L"%s =>\n\t%s\n", item.PhydiskName.c_str(), item.DevPath.c_str());
    }
    wprintf(L"\n");
}

void TurnOnDiskPerf(std::wstring devpath)
{
    wprintf(L"TurnOn diskperf for %s\n", devpath.c_str());
    HANDLE device = CreateFile(devpath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (INVALID_HANDLE_VALUE != device)
    {
        DWORD return_size = 0;
        BYTE buffer[256] = {0};
        BOOL ok = DeviceIoControl(device,
            IOCTL_DISK_PERFORMANCE,
            NULL,
            0,
            buffer,
            256,
            &return_size,
            NULL);

        if(!ok)
            wprintf(L"**Failed to turn-on DISKPERF for %s\n", devpath.c_str());
        wprintf(L"LastError=%d, return_size=%d\n", GetLastError(), return_size);
        CloseHandle(device);
    }
    else
        wprintf(L"failed to open devpath\n");
}
void TurnOffDiskPerf(std::wstring devpath) 
{
    wprintf(L"TurnOff diskperf for %s\n", devpath.c_str());

    HANDLE device = CreateFile(devpath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (INVALID_HANDLE_VALUE != device)
    {
        DWORD return_size = 0;
        BOOL ok = DeviceIoControl(device,
            IOCTL_DISK_PERFORMANCE_OFF,
            NULL,
            0,
            NULL,
            0,
            &return_size,
            NULL);

        if (!ok)
            wprintf(L"**Failed to turn-off DISKPERF for %s\n", devpath.c_str());
        wprintf(L"LastError=%d, return_size=%d\n", GetLastError(), return_size);
        CloseHandle(device);
    }
    else
        wprintf(L"failed to open devpath\n");
}

void Usage()
{
    wprintf(L"This app is used to turn on or turn off PerformanceCounter of all disk devices.\n");
    wprintf(L"Usage: IoctlDiskPerfOnOff [switch]\n");
    wprintf(L"       [switch] ==> on or off\n");
    wprintf(L"Example: IoctlDiskPerfOnOff on\n");
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        Usage();
        return -1;
    }
    bool do_turnon = (0==_stricmp(argv[1], "on"));

    std::list<PHYDISK_INFO> disklist;
    EnumPhysicalDisks(disklist);
    if(disklist.size() > 0)
    {
        PrintDiskList(disklist);
    }

    wprintf(L"\n\n");
    for (auto disk : disklist)
    {
        if(do_turnon)
        {
            TurnOnDiskPerf(disk.PhydiskName);
            TurnOnDiskPerf(disk.DevPath);
        }
        else
        {
            TurnOffDiskPerf(disk.PhydiskName);
            TurnOffDiskPerf(disk.DevPath);
        }
    }
}
