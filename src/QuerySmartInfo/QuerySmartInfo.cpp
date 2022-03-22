// QuerySmartInfo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <Setupapi.h>
#include <Ntddstor.h>
#include <iostream>
#include <initguid.h>
#include <stdio.h>
#include <string>
#include <list>

#include "NVMe_SMART_Info.h"

#pragma comment( lib, "setupapi.lib" )

//common LOG PAGE ID by NVMe spec
typedef enum {
    NVME_LOG_PAGE_ERROR_INFO = 0x01,
    NVME_LOG_PAGE_HEALTH_INFO = 0x02,
    NVME_LOG_PAGE_FIRMWARE_SLOT_INFO = 0x03,
    NVME_LOG_PAGE_CHANGED_NAMESPACE_LIST = 0x04,
    NVME_LOG_PAGE_COMMAND_EFFECTS = 0x05,
    NVME_LOG_PAGE_DEVICE_SELF_TEST = 0x06,
    NVME_LOG_PAGE_TELEMETRY_HOST_INITIATED = 0x07,
    NVME_LOG_PAGE_TELEMETRY_CTLR_INITIATED = 0x08,
    NVME_LOG_PAGE_ENDURANCE_GROUP_INFORMATION = 0x09,

    NVME_LOG_PAGE_RESERVATION_NOTIFICATION = 0x80,
    NVME_LOG_PAGE_SANITIZE_STATUS = 0x81,

} NVME_LOG_PAGES;


#define PHYSICAL_DISK_FORMAT    L"\\\\.\\PhysicalDrive%d"

using namespace std;

BOOL ListPhysicalDisk(list<wstring> &result)
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
            BYTE buffer[1024] = {0};

            ifdetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buffer;
            ifdetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            ok = SetupDiGetDeviceInterfaceDetail(devinfo, &ifdata, ifdetail, need_size, &need_size, NULL);
            if (TRUE == ok)
            {
                HANDLE device = CreateFile(ifdetail->DevicePath, GENERIC_READ | GENERIC_WRITE, 
                                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
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
                        WCHAR name[1024] = {0};
                        _snwprintf_s(name, 1024, PHYSICAL_DISK_FORMAT, disk_number.DeviceNumber);
                        result.push_back(name);
                    }
                    CloseHandle(device);
                }
            }
        }
    }

    if(result.size() > 0)
        return TRUE;
    return FALSE;
}
BOOL ReadSmartRawData(const wstring &diskname, BYTE *buffer, DWORD buffer_Size)
{
    HANDLE disk = CreateFile(diskname.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    BOOL ok = FALSE;
    if(disk != INVALID_HANDLE_VALUE)
    {
        PSTORAGE_PROPERTY_QUERY query = (PSTORAGE_PROPERTY_QUERY) buffer;
        query->QueryType = PropertyStandardQuery;
        query->PropertyId = StorageAdapterProtocolSpecificProperty;
        PSTORAGE_PROTOCOL_SPECIFIC_DATA pdata = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)&query->AdditionalParameters;
        DWORD ret_size=0;

        pdata->ProtocolType = ProtocolTypeNvme; //ProtocolTypeAta for BlkFw Device
        pdata->DataType = NVMeDataTypeLogPage;  //STORAGE_PROTOCOL_ATA_DATA_TYPE::AtaDataTypeLogPage for BlkFw Device
        pdata->ProtocolDataRequestValue = NVME_LOG_PAGE_HEALTH_INFO;
        pdata->ProtocolDataRequestSubValue = 0;
        pdata->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA); //where should the returned data placed? offset from begin of PSTORAGE_PROTOCOL_SPECIFIC_DATA
        pdata->ProtocolDataLength = buffer_Size-sizeof(STORAGE_PROPERTY_QUERY) -sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
        //buffer_Size - sizeof(STORAGE_PROPERTY_ID) - sizeof(STORAGE_QUERY_TYPE); //sizeof AdditionalParameters array

        ok = DeviceIoControl(disk, IOCTL_STORAGE_QUERY_PROPERTY, query, 
                                    buffer_Size, query, buffer_Size, &ret_size, NULL);
        if(!ok)
            wprintf(L"\t DeviceIoControl() failed, LastError=%d\n", GetLastError());
        CloseHandle(disk);
    }

    return ok;
}
void ParseSmartInfo(wstring &result, BYTE *buffer)
{
    PSTORAGE_PROPERTY_QUERY query = (PSTORAGE_PROPERTY_QUERY)buffer;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA pdata = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)&query->AdditionalParameters;
    PNVME_SMART_INFO info = (PNVME_SMART_INFO) ((PUCHAR)pdata + pdata->ProtocolDataOffset);

    WCHAR msg[256] = {0};
    _snwprintf_s(msg, _countof(msg), L"\tAvailableSpaceLow=%d, TemperatureExceed=%d, ReliabilityError=%d, ReadOnlyMode=%d, VolatileBackupFailed=%d\n", 
                info->CriticalWarn.Warn.AvailableSpaceLow, info->CriticalWarn.Warn.TemperatureExceed, 
                info->CriticalWarn.Warn.ReliabilityError, info->CriticalWarn.Warn.ReadOnlyMode, 
                info->CriticalWarn.Warn.VolatileBackupFailed);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tOverallTemperature=%X %X\n", info->OverallTemperature[0], info->OverallTemperature[1]);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tAvailableSpare=%X\n", info->AvailableSpare);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tAvailableSpareThreshold=%X\n", info->AvailableSpareThreshold);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tPercentageUsed=%X\n", info->PercentageUsed);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tDataUnitRead=%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
                    info->DataUnitRead[0], info->DataUnitRead[1], info->DataUnitRead[2], info->DataUnitRead[3], 
                    info->DataUnitRead[4], info->DataUnitRead[5], info->DataUnitRead[6], info->DataUnitRead[7],
                    info->DataUnitRead[8], info->DataUnitRead[9], info->DataUnitRead[10], info->DataUnitRead[11],
                    info->DataUnitRead[12], info->DataUnitRead[13], info->DataUnitRead[14], info->DataUnitRead[15]);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tDataUnitWrite=%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
                    info->DataUnitWrite[0], info->DataUnitWrite[1], info->DataUnitWrite[2], info->DataUnitWrite[3],
                    info->DataUnitWrite[4], info->DataUnitWrite[5], info->DataUnitWrite[6], info->DataUnitWrite[7],
                    info->DataUnitWrite[8], info->DataUnitWrite[9], info->DataUnitWrite[10], info->DataUnitWrite[11],
                    info->DataUnitWrite[12], info->DataUnitWrite[13], info->DataUnitWrite[14], info->DataUnitWrite[15]);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tHostReadCommands=%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
                    info->HostReadCommands[0], info->HostReadCommands[1], info->HostReadCommands[2], info->HostReadCommands[3],
                    info->HostReadCommands[4], info->HostReadCommands[5], info->HostReadCommands[6], info->HostReadCommands[7],
                    info->HostReadCommands[8], info->HostReadCommands[9], info->HostReadCommands[10], info->HostReadCommands[11],
                    info->HostReadCommands[12], info->HostReadCommands[13], info->HostReadCommands[14], info->HostReadCommands[15]);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tHostWrittenCommands=%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
                    info->HostWrittenCommands[0], info->HostWrittenCommands[1], info->HostWrittenCommands[2], info->HostWrittenCommands[3],
                    info->HostWrittenCommands[4], info->HostWrittenCommands[5], info->HostWrittenCommands[6], info->HostWrittenCommands[7],
                    info->HostWrittenCommands[8], info->HostWrittenCommands[9], info->HostWrittenCommands[10], info->HostWrittenCommands[11],
                    info->HostWrittenCommands[12], info->HostWrittenCommands[13], info->HostWrittenCommands[14], info->HostWrittenCommands[15]);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tControllerBusyTime=%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
                    info->ControllerBusyTime[0], info->ControllerBusyTime[1], info->ControllerBusyTime[2], info->ControllerBusyTime[3],
                    info->ControllerBusyTime[4], info->ControllerBusyTime[5], info->ControllerBusyTime[6], info->ControllerBusyTime[7],
                    info->ControllerBusyTime[8], info->ControllerBusyTime[9], info->ControllerBusyTime[10], info->ControllerBusyTime[11],
                    info->ControllerBusyTime[12], info->ControllerBusyTime[13], info->ControllerBusyTime[14], info->ControllerBusyTime[15]);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tPowerCycle=%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
                    info->PowerCycle[0], info->PowerCycle[1], info->PowerCycle[2], info->PowerCycle[3],
                    info->PowerCycle[4], info->PowerCycle[5], info->PowerCycle[6], info->PowerCycle[7],
                    info->PowerCycle[8], info->PowerCycle[9], info->PowerCycle[10], info->PowerCycle[11],
                    info->PowerCycle[12], info->PowerCycle[13], info->PowerCycle[14], info->PowerCycle[15]);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tPowerOnHours=%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
                    info->PowerOnHours[0], info->PowerOnHours[1], info->PowerOnHours[2], info->PowerOnHours[3],
                    info->PowerOnHours[4], info->PowerOnHours[5], info->PowerOnHours[6], info->PowerOnHours[7],
                    info->PowerOnHours[8], info->PowerOnHours[9], info->PowerOnHours[10], info->PowerOnHours[11],
                    info->PowerOnHours[12], info->PowerOnHours[13], info->PowerOnHours[14], info->PowerOnHours[15]);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tUnsafeShutdowns=%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
                    info->UnsafeShutdowns[0], info->UnsafeShutdowns[1], info->UnsafeShutdowns[2], info->UnsafeShutdowns[3],
                    info->UnsafeShutdowns[4], info->UnsafeShutdowns[5], info->UnsafeShutdowns[6], info->UnsafeShutdowns[7],
                    info->UnsafeShutdowns[8], info->UnsafeShutdowns[9], info->UnsafeShutdowns[10], info->UnsafeShutdowns[11],
                    info->UnsafeShutdowns[12], info->UnsafeShutdowns[13], info->UnsafeShutdowns[14], info->UnsafeShutdowns[15]);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tMediaErrors=%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
                    info->MediaErrors[0], info->MediaErrors[1], info->MediaErrors[2], info->MediaErrors[3],
                    info->MediaErrors[4], info->MediaErrors[5], info->MediaErrors[6], info->MediaErrors[7],
                    info->MediaErrors[8], info->MediaErrors[9], info->MediaErrors[10], info->MediaErrors[11],
                    info->MediaErrors[12], info->MediaErrors[13], info->MediaErrors[14], info->MediaErrors[15]);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tErrorInfoLogEntryCount=%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
                    info->ErrorInfoLogEntryCount[0], info->ErrorInfoLogEntryCount[1], info->ErrorInfoLogEntryCount[2], info->ErrorInfoLogEntryCount[3],
                    info->ErrorInfoLogEntryCount[4], info->ErrorInfoLogEntryCount[5], info->ErrorInfoLogEntryCount[6], info->ErrorInfoLogEntryCount[7],
                    info->ErrorInfoLogEntryCount[8], info->ErrorInfoLogEntryCount[9], info->ErrorInfoLogEntryCount[10], info->ErrorInfoLogEntryCount[11],
                    info->ErrorInfoLogEntryCount[12], info->ErrorInfoLogEntryCount[13], info->ErrorInfoLogEntryCount[14], info->ErrorInfoLogEntryCount[15]);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tWarningCompositeTemperatureTime=%X\n", info->WarningCompositeTemperatureTime);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tCriticalCompositeTemperatureTime=%X\n", info->CriticalCompositeTemperatureTime);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tTemperatureSensor1=%d\n", info->TemperatureSensor1);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tTemperatureSensor2=%d\n", info->TemperatureSensor2);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tTemperatureSensor3=%d\n", info->TemperatureSensor3);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tTemperatureSensor4=%d\n", info->TemperatureSensor4);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tTemperatureSensor5=%d\n", info->TemperatureSensor5);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tTemperatureSensor6=%d\n", info->TemperatureSensor6);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tTemperatureSensor7=%d\n", info->TemperatureSensor7);
    result = result + msg;

    ZeroMemory(msg, sizeof(msg));
    _snwprintf_s(msg, _countof(msg), L"\tTemperatureSensor8=%d\n", info->TemperatureSensor8);
    result = result + msg;
}

int main()
{
    list<wstring> disklist;
    BOOL ok = FALSE;

    ok = ListPhysicalDisk(disklist);
    if(ok == TRUE)
    {
        for (const auto& diskname : disklist)
        {
            BYTE buffer[4096] = {0};
            BOOL ok = ReadSmartRawData(diskname, buffer, 4096);
            if(TRUE == ok)
            {
                wstring msg = L"";
                ParseSmartInfo(msg, buffer);
                wprintf(L"[%s]\n%s\n", diskname.c_str(), msg.c_str());
            }
            else
                wprintf(L"[Error] (%s) can't get SMART data. skipping... \n", diskname.c_str());
        }
    }
    else
    {
    wprintf(L"No physical disk found...\n");
    }
}
