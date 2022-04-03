// DriverBindingTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <SetupAPI.h>
#include <iostream>
#include <initguid.h>
#include <devpkey.h>
#include <string>
#include <list>
using namespace std;
#pragma comment( lib, "setupapi.lib" )

//DEFINE_DEVPROPKEY(DEVPKEY_Device_DriverDate, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 2);     // DEVPROP_TYPE_FILETIME
//DEFINE_DEVPROPKEY(DEVPKEY_Device_DriverVersion, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 3);     // DEVPROP_TYPE_STRING
//DEFINE_DEVPROPKEY(DEVPKEY_Device_DriverDesc, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 4);     // DEVPROP_TYPE_STRING
//DEFINE_DEVPROPKEY(DEVPKEY_Device_DriverInfPath, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 5);     // DEVPROP_TYPE_STRING
//DEFINE_DEVPROPKEY(DEVPKEY_Device_DriverInfSection, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 6);     // DEVPROP_TYPE_STRING
//DEFINE_DEVPROPKEY(DEVPKEY_Device_DriverInfSectionExt, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 7);     // DEVPROP_TYPE_STRING
//DEFINE_DEVPROPKEY(DEVPKEY_Device_MatchingDeviceId, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 8);     // DEVPROP_TYPE_STRING
//DEFINE_DEVPROPKEY(DEVPKEY_Device_DriverProvider, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 9);     // DEVPROP_TYPE_STRING
//DEFINE_DEVPROPKEY(DEVPKEY_Device_DriverPropPageProvider, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 10);    // DEVPROP_TYPE_STRING
//DEFINE_DEVPROPKEY(DEVPKEY_Device_DriverCoInstallers, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 11);    // DEVPROP_TYPE_STRING_LIST
//DEFINE_DEVPROPKEY(DEVPKEY_Device_ResourcePickerTags, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 12);    // DEVPROP_TYPE_STRING
//DEFINE_DEVPROPKEY(DEVPKEY_Device_ResourcePickerExceptions, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 13);    // DEVPROP_TYPE_STRING
//DEFINE_DEVPROPKEY(DEVPKEY_Device_DriverRank, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 14);    // DEVPROP_TYPE_UINT32
//DEFINE_DEVPROPKEY(DEVPKEY_Device_DriverLogoLevel, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 15);    // DEVPROP_TYPE_UINT32


void ListBindedDriverList(wstring devpath)
{
    HDEVINFO infoset;
    SP_DEVINFO_DATA infodata;
    SP_DEVICE_INTERFACE_DATA devif;
    DWORD index;
    DWORD error = 0;
    DWORD need_size;
    int err = 0;
    bool graid_binded;
    DEVPROPTYPE PropType = 0;
    WCHAR Buffer[4096] = { 0 };

    infoset = SetupDiCreateDeviceInfoList(NULL, NULL);
    if (INVALID_HANDLE_VALUE == infoset) {
        error = GetLastError();
        goto out;
    }

    ZeroMemory(&devif, sizeof(SP_DEVICE_INTERFACE_DATA));
    devif.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (!SetupDiOpenDeviceInterface(infoset, devpath.c_str(), 0, &devif)) {
        error = GetLastError();
        goto out;
    }

    ZeroMemory(&infodata, sizeof(SP_DEVINFO_DATA));
    infodata.cbSize = sizeof(SP_DEVINFO_DATA);

    if (!SetupDiGetDeviceInterfaceDetail(infoset, &devif, NULL, 0, &need_size, &infodata)) {
        error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER) {
            goto out;
        }
    }

    SetupDiDeleteInterfaceDeviceData(infoset, &devif);

    if (!SetupDiGetDeviceProperty(
        infoset,
        &infodata,
        &DEVPKEY_Device_DriverDesc,
        &PropType,
        (BYTE*)Buffer,
        sizeof(Buffer),
        NULL, 0) || PropType != DEVPROP_TYPE_STRING) {

        error = GetLastError();
        goto out;
    }

    if (!SetupDiBuildDriverInfoList(infoset, &infodata, SPDIT_COMPATDRIVER)) {
        error = GetLastError();
        goto out;
    }

    SP_DRVINFO_DATA drvinfo;
    ZeroMemory(&drvinfo, sizeof(SP_DRVINFO_DATA));
    drvinfo.cbSize = sizeof(SP_DRVINFO_DATA);
    index = 0;
    while (SetupDiEnumDriverInfo(infoset, &infodata, SPDIT_COMPATDRIVER, index, &drvinfo)) {
        ++index;

        SYSTEMTIME systime = {0};
        FileTimeToSystemTime(&drvinfo.DriverDate, &systime);
        wprintf(L"[%s]\n", drvinfo.Description);
        wprintf(L"  Type=%d, DriverVersion=%lld, DriverDate=%d-%d-%d %d:%d:%d\n", drvinfo.DriverType, drvinfo.DriverVersion, 
                    systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond);
        wprintf(L"  MfgName=%s, ProviderName=%s\n", drvinfo.MfgName, drvinfo.ProviderName);
    }
    SetupDiDestroyDriverInfoList(infoset, &infodata, SPDIT_COMPATDRIVER);
    SetupDiDeleteDeviceInterfaceData(infoset, &devif);
    error = 0;
out:
    if(error != 0)
        wprintf(L"error occurred, LastError=%d\n", error);
    if(infoset != NULL && infoset != INVALID_HANDLE_VALUE)
        SetupDiDestroyDeviceInfoList(infoset);
    return;
}

BOOLEAN ListStorportAdapters(list<wstring> &result)
{
    HDEVINFO devinfo = NULL;
    GUID disk_class_guid = GUID_DEVINTERFACE_STORAGEPORT;

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
            BYTE buffer[1024] = { 0 };

            ifdetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buffer;
            ifdetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            ok = SetupDiGetDeviceInterfaceDetail(devinfo, &ifdata, ifdetail, need_size, &need_size, NULL);
            if (TRUE == ok)
            {
                HANDLE device = CreateFile(ifdetail->DevicePath, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                if (INVALID_HANDLE_VALUE != device)
                {
                    result.push_back(ifdetail->DevicePath);
                    CloseHandle(device);
                }
            }
        }
    }

    if (result.size() > 0)
        return TRUE;
    return FALSE;
}

void ShowUsage()
{
    wprintf(L"Please assign devpath of target device which you want to list binded driver.\n");
}

int main(wchar_t *argv[], int argc)
{
    list<wstring> devlist;
    ListStorportAdapters(devlist);
    for(const auto& devpath : devlist)
        ListBindedDriverList(devpath);
}

