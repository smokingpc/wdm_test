// QueryNQNtoBDF.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <windows.h>
#include <SetupAPI.h>
#include <iostream>
#include <initguid.h>
#include <devpkey.h>
#include <string>
#include <list>
#include "nvme_structure.h"
using namespace std;

#pragma comment(lib, "setupapi.lib")

void Usage()
{
    printf("Usage: QueryNQNtoBDF.exe [keyword in nvme device manufacturer] \n");
}

static inline void string_trim(string& str)
{
    std::size_t found = str.find_last_not_of(" ");
    if (found != string::npos)
        str.erase(found + 1);
    else
        str.clear();            // str is all whitespace
}


wstring BuildNqn(PNVME_IDENTIFY_CONTROLLER_DATA14 identify)
{
    wstring ret = L"";
    string subnqn = "";
    string model = "";
    string serial = "";
    string fw_rev = "";

    subnqn.assign((char*)identify->SUBNQN);
    if (subnqn == "" || identify->VER == 0)
    {
        int offset = snprintf((char*)identify->SUBNQN, 256,
            "nqn.2014.08.org.nvmexpress:%04x%04x",
            identify->VID, identify->SSVID);

        memcpy((char*)identify->SUBNQN + offset, identify->SN, sizeof(identify->SN));
        offset += sizeof(identify->SN);
        memcpy((char*)identify->SUBNQN + offset, identify->MN, sizeof(identify->MN));
        offset += sizeof(identify->MN);
        memset((char*)identify->SUBNQN + offset, 0, sizeof(identify->SUBNQN) - offset);
        subnqn = (char*)identify->SUBNQN;
    }
    string_trim(subnqn);
    ret.resize(subnqn.length());
#pragma warning(suppress : 4996)
    mbstowcs(&ret[0], &subnqn[0], subnqn.length());
    return ret;
}

wstring QueryNVMeDeviceNqn(const wchar_t *devpath)
{
    wstring ret = L"";

    HANDLE hDevice = CreateFile(devpath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hDevice != INVALID_HANDLE_VALUE) 
    {
        PSTORAGE_PROPERTY_QUERY query = NULL;
        PSTORAGE_PROTOCOL_SPECIFIC_DATA data = NULL;
        PSTORAGE_PROTOCOL_DATA_DESCRIPTOR desc = NULL;

        DWORD size = FIELD_OFFSET(STORAGE_PROPERTY_QUERY, AdditionalParameters) + 
                    sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) + 
                    sizeof(NVME_IDENTIFY_CONTROLLER_DATA14);
        DWORD ret_size = 0;

        PBYTE buffer = new BYTE[size];
        ZeroMemory(buffer, size);

        query = (PSTORAGE_PROPERTY_QUERY) buffer;
        desc = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR) buffer;
        data = (PSTORAGE_PROTOCOL_SPECIFIC_DATA) query->AdditionalParameters;

        query->PropertyId = StorageAdapterProtocolSpecificProperty;
        query->QueryType = PropertyStandardQuery;

        data->ProtocolType = ProtocolTypeNvme;
        data->DataType = NVMeDataTypeIdentify;
        data->ProtocolDataRequestValue = NVME_IDENTIFY_CNS_CONTROLLER;
        data->ProtocolDataRequestSubValue = 0;
        data->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
        data->ProtocolDataLength = sizeof(NVME_IDENTIFY_CONTROLLER_DATA14);

        BOOL ok = DeviceIoControl(hDevice,
            IOCTL_STORAGE_QUERY_PROPERTY,
            buffer,
            size,
            buffer,
            size,
            &ret_size,
            NULL);


        if(!ok)
            wprintf(L"DeviceIoControl failed, LastError=%d\n", GetLastError());

        if ((desc->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
            (desc->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))) 
        {
            wprintf(L"Invalid return data from DeviceIoControl\n");
            ok = FALSE;
        }

        data = &desc->ProtocolSpecificData;

        PNVME_IDENTIFY_CONTROLLER_DATA14 identify = (PNVME_IDENTIFY_CONTROLLER_DATA14)((PCHAR)data + data->ProtocolDataOffset);

        if ((identify->VID == 0) || (identify->NN == 0)) 
        {
            wprintf(L"Invalid NVME_IDENTIFY_CONTROLLER_DATA\n");
            ok = FALSE;
        }

        if(ok)
        {
            ret = BuildNqn(identify);
        }
        //CopyMemory(id, identifyControllerData, sizeof(NVME_IDENTIFY_CONTROLLER_DATA14));

        delete[] buffer;
        CloseHandle(hDevice);
    }
    else
        wprintf(L"failed to open %s, skip...\n", devpath);

    return ret;
}

void ListNvmeNqnWithPciBDF(wchar_t *keyword)
{
    HDEVINFO infoset;
    SP_DEVINFO_DATA infodata;
    SP_DEVICE_INTERFACE_DATA ifdata;
    DWORD id;
    const GUID* class_guid = &GUID_DEVINTERFACE_STORAGEPORT;
    infoset = SetupDiGetClassDevsW(
        class_guid,
        NULL,
        NULL,
        DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if(INVALID_HANDLE_VALUE != infoset)
    {
        ZeroMemory(&infodata, sizeof(SP_DEVINFO_DATA));
        infodata.cbSize = sizeof(SP_DEVINFO_DATA);
        id = 0;

        while (SetupDiEnumDeviceInfo(infoset, id++, &infodata))
        {
            ZeroMemory(&ifdata, sizeof(SP_DEVICE_INTERFACE_DATA));
            ifdata.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
            
            const DEVPROPKEY *key = &DEVPKEY_Device_Manufacturer;
            DEVPROPTYPE type = 0;
            WCHAR buffer[512] = {0};
            wstring devpath = L"";
            wstring locinfo = L"";
            wstring nqn = L"";

            if(!SetupDiGetDevicePropertyW(infoset, &infodata, key, &type, (PBYTE) buffer, 512, NULL, 0))
            {
                wprintf(L"SetupDiGetDevicePropertyW failed(error=%d), try next device...\n", GetLastError());
            }

            //if manufacturer keyword is assigned, we have to check manufacturer info to filter device.
            if(NULL != keyword && wcslen(keyword)>0)
            {
                if(NULL == wcsstr(buffer, keyword))
                    continue;
            }

            memset(buffer, 0, _countof(buffer) * sizeof(WCHAR));
            key = &DEVPKEY_Device_LocationInfo;
            if (!SetupDiGetDevicePropertyW(infoset, &infodata, key, &type, (PBYTE)buffer, 512, NULL, 0))
            {
                wprintf(L"SetupDiGetDevicePropertyW failed(error=%d), try next device...\n", GetLastError());
            }
            locinfo = buffer;

            if (!SetupDiEnumDeviceInterfaces(infoset, &infodata, class_guid, 0, &ifdata))
            {
                wprintf(L"Enum DeviceInterface failed(error=%d), try next device...\n", GetLastError());
                continue;
            }

            if ((ifdata.Flags & SPINT_ACTIVE) == 0)
            {
                wprintf(L"Got interface[0] but inactive, try next device...\n");
                continue;
            }
            
            PSP_DEVICE_INTERFACE_DETAIL_DATA detail;
            DWORD need_size = 0;

            SetupDiGetDeviceInterfaceDetail(
                infoset,
                &ifdata,
                NULL,
                0,
                &need_size,
                NULL);

            detail = (SP_DEVICE_INTERFACE_DETAIL_DATA*) new BYTE[need_size];
            if (NULL == detail)
            {
                wprintf(L"Allocate Buffer Failed, try next device...\n");
                continue;
            }
            //wprintf(L"required buffer size = %d\n", need_size);
            ZeroMemory(detail, need_size);
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            if (!SetupDiGetDeviceInterfaceDetail(
                infoset, &ifdata, detail, need_size,
                NULL, NULL))
            {
                wprintf(L"SetupDiGetDeviceInterfaceDetail Failed, error=%d...\n", GetLastError());
                continue;
            }

            devpath = detail->DevicePath;
            nqn = QueryNVMeDeviceNqn(devpath.c_str());

            wprintf(L"%s => %s , from %s\n", nqn.c_str(), locinfo.c_str(), devpath.c_str());
        }

        SetupDiDestroyDeviceInfoList(infoset);
    }
}

int wmain(int argc, wchar_t* argv[])
{
    wchar_t *keyword = NULL;
    if(argc >=2)
        keyword = argv[1];

    ListNvmeNqnWithPciBDF(keyword);
}
