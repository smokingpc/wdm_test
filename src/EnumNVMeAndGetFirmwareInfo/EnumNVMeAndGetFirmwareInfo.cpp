// EnumNVMeAndGetFirmwareInfo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <ntddscsi.h>
#include <SetupAPI.h>
#include <iostream>
#include <initguid.h>
#include <devpkey.h>
#include <string>
#include <list>

#pragma comment( lib, "setupapi.lib" )
#define PHYSICAL_DISK_FORMAT    L"\\\\?\\PhysicalDrive%d"

using namespace std;

typedef struct _PHYSICAL_DISK_DEVINFO
{
    wstring PhyDiskName = L"";
    wstring DevicePath = L"";
}PHYDISK_DEVINFO, *PPHYSICAL_DISK_INFO;

size_t EnumPhysicalDisks(list<PHYDISK_DEVINFO> &disklist)
{
    HDEVINFO devinfo = NULL;
    GUID disk_class_guid = GUID_DEVINTERFACE_DISK;
    GUID* debug = &disk_class_guid;
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
                        WCHAR diskname[MAX_PATH] = {0};
                        PHYDISK_DEVINFO info;
                        wsprintf(diskname, PHYSICAL_DISK_FORMAT, disk_number.DeviceNumber);
                        info.DevicePath = ifdetail->DevicePath;
                        info.PhyDiskName = diskname;
                        disklist.push_back(info);
                    }

                    CloseHandle(device);
                }
                //else
                //    printf("CreateFile() failed, LastError=%d\n", GetLastError());
            }
            //else
            //    printf("SetupDiGetDeviceInterfaceDetail() failed, LastError=%d\n", GetLastError());
            delete[] buffer;
        }
    }

    return disklist.size();
}

void PrintFirmwareInfo(PSTORAGE_FIRMWARE_INFO info)
{
    wprintf(L"\tUpgradeSupport=%s\n", (info->UpgradeSupport?L"TRUE":L"FALSE"));
    wprintf(L"\tSlotCount=0x%02X, ActiveSlot=0x%02X, PendingActivateSlot=0x%02X\n", info->SlotCount, info->ActiveSlot, info->PendingActivateSlot);
    
    for(int i=0; i< info->SlotCount; i++)
    {
        STORAGE_FIRMWARE_SLOT_INFO *slot = &info->Slot[i];

        wprintf(L"\t(Slot %d) Readonly=%s, Revision=0x%016llx,", slot->SlotNumber, (slot->ReadOnly?L"TRUE":L"FALSE"), slot->Revision.AsUlonglong);
        wprintf(L"%S\n", (char*)slot->Revision.Info);
    }
}
void PrintFirmwareInfo(PSTORAGE_FIRMWARE_INFO_V2 info) 
{
    wprintf(L"\tUpgradeSupport=%s\n", (info->UpgradeSupport ? L"TRUE" : L"FALSE"));
    wprintf(L"\tSlotCount=0x%02X, ActiveSlot=0x%02X, PendingActivateSlot=0x%02X\n", info->SlotCount, info->ActiveSlot, info->PendingActivateSlot);

    for (int i = 0; i < info->SlotCount; i++)
    {
        STORAGE_FIRMWARE_SLOT_INFO_V2* slot = &info->Slot[i];

        wprintf(L"\t(Slot %d) Readonly=%s,", slot->SlotNumber, (slot->ReadOnly ? L"TRUE" : L"FALSE"));
        
        for(int j=0; j< STORAGE_FIRMWARE_SLOT_INFO_V2_REVISION_LENGTH; j++)
            wprintf(L"%02X ", slot->Revision[j]);
        wprintf(L", %S", (char*)slot->Revision);
        wprintf(L"\n");
    }
}

void QueryNVMeFirmwareInfo(const PHYDISK_DEVINFO &diskinfo)
{
    PSRB_IO_CONTROL         ioctrl = NULL;
    PFIRMWARE_REQUEST_BLOCK request = NULL;
    //PSTORAGE_FIRMWARE_INFO  info = NULL;

    BYTE buffer[4096] = {0};
    ULONG bufsize = sizeof(buffer);
    ioctrl = (PSRB_IO_CONTROL)buffer;
    request = (PFIRMWARE_REQUEST_BLOCK)(ioctrl + 1);

    ioctrl->HeaderLength = sizeof(SRB_IO_CONTROL);
    ioctrl->ControlCode = IOCTL_SCSI_MINIPORT_FIRMWARE;
    RtlMoveMemory(ioctrl->Signature, IOCTL_MINIPORT_SIGNATURE_FIRMWARE, 8);
    ioctrl->Timeout = 30;
    ioctrl->Length = bufsize - sizeof(SRB_IO_CONTROL);
    
    ULONG offset = ((sizeof(SRB_IO_CONTROL) + sizeof(FIRMWARE_REQUEST_BLOCK) - 1) / sizeof(PVOID) + 1) * sizeof(PVOID);
    request->Version = FIRMWARE_REQUEST_BLOCK_STRUCTURE_VERSION;
    request->Size = sizeof(FIRMWARE_REQUEST_BLOCK);
    request->Function = FIRMWARE_FUNCTION_GET_INFO;
    request->Flags = FIRMWARE_REQUEST_FLAG_CONTROLLER;
    request->DataBufferOffset = offset;
    request->DataBufferLength = bufsize - offset;

    HANDLE hdisk = CreateFile(diskinfo.PhyDiskName.c_str(), 
                    GENERIC_READ|GENERIC_WRITE, 
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(INVALID_HANDLE_VALUE == hdisk)
        return;
    ULONG ret_size = 0;
    BOOL ok = DeviceIoControl(hdisk,
        IOCTL_SCSI_MINIPORT,
        buffer, bufsize,
        buffer, bufsize,
        &ret_size, NULL);

    if(ok)
    {
        //print firmware information
        ULONG *ver = (ULONG *)((PUCHAR)ioctrl + request->DataBufferOffset);
        wprintf(L"[%s]\n", diskinfo.PhyDiskName.c_str());
        if(*ver == STORAGE_FIRMWARE_INFO_STRUCTURE_VERSION)
        {
            PrintFirmwareInfo((PSTORAGE_FIRMWARE_INFO)((PUCHAR)ioctrl + request->DataBufferOffset));
        }
        else if (*ver == STORAGE_FIRMWARE_INFO_STRUCTURE_VERSION_V2)
        {
            PrintFirmwareInfo((PSTORAGE_FIRMWARE_INFO_V2)((PUCHAR)ioctrl + request->DataBufferOffset));
        }
    }
    CloseHandle(hdisk);
}

int main()
{
    list<PHYDISK_DEVINFO> disklist;
    int count = EnumPhysicalDisks(disklist);

    for(const auto &disk : disklist)
    {
        //wprintf(L"%s=>%s\n", disk.DevicePath.c_str(), disk.PhyDiskName.c_str());
        QueryNVMeFirmwareInfo(disk);
    }
}
