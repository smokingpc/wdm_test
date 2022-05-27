// FindDeviceNodeByHwId.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <tchar.h>
#include <windows.h>
#include <SetupAPI.h>
#include <iostream>
#include <initguid.h>
#include <devpkey.h>
#include <devpropdef.h>
#include <string>
#include <list>
#include "DevicePropertyData.h"

#pragma comment( lib, "setupapi.lib" )

#define BIG_BUFFER_SIZE     4096
#define SMALL_BUFFER_SIZE   256
#define FORMAT_16_BYTES     _T("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X")
#define FORMAT_SYSTEMTIME   _T("%04d/%02d/%02d %02d:%02d:%02d.%03d")
using namespace std;

__inline void ParsePropertyToString(BYTE *data, DEVPROPTYPE type, tstring& result)
{
    switch (type)
    {
    case DEVPROP_TYPE_BOOLEAN:
        {
            result = (data[0] > 0)? _T("TRUE"): _T("FALSE");
        }
        break;

    case DEVPROP_TYPE_SBYTE:
        {
            TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
            _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((char*)data)[0]);
            result = buf;
        }
        break;
    case DEVPROP_TYPE_BYTE:
        {
            TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
            _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((unsigned char*)data)[0]);
            result = buf;
        }
        break;
    case DEVPROP_TYPE_INT16:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((short*)data)[0]);
        result = buf;
    }
        break;
    case DEVPROP_TYPE_UINT16:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((unsigned short*)data)[0]);
        result = buf;
    }
        break;
    case DEVPROP_TYPE_INT32:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((int*)data)[0]);
        result = buf;
    }
        break;
    case DEVPROP_TYPE_UINT32:
        {
            TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
            _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((unsigned int*)data)[0]);
            result = buf;
        }
        break;
    case DEVPROP_TYPE_INT64:
    case DEVPROP_TYPE_CURRENCY:     //64bit signed int
        {
            TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
            _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%lld"), ((INT64 *)data)[0]);
            result = buf;
        }
        break;
    case DEVPROP_TYPE_UINT64:
        {
            TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
            _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%lld"), ((UINT64*)data)[0]);
            result = buf;
        }
        break;
    case DEVPROP_TYPE_FLOAT:
        {
            TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
            _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%f"), ((float*)data)[0]);
            result = buf;
        }
        break;
    case DEVPROP_TYPE_DOUBLE:
        {
            TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
            _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%lf"), ((double*)data)[0]);
            result = buf;
        }
        break;
    case DEVPROP_TYPE_DECIMAL:      //128bit int
    case DEVPROP_TYPE_GUID:
        {
            TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
            _stprintf_s(buf, SMALL_BUFFER_SIZE, FORMAT_16_BYTES, 
                ((TCHAR*)data)[0], ((TCHAR*)data)[1], ((TCHAR*)data)[2], ((TCHAR*)data)[3], 
                ((TCHAR*)data)[4], ((TCHAR*)data)[5], ((TCHAR*)data)[6], ((TCHAR*)data)[7],
                ((TCHAR*)data)[8], ((TCHAR*)data)[9], ((TCHAR*)data)[10], ((TCHAR*)data)[11],
                ((TCHAR*)data)[12], ((TCHAR*)data)[13], ((TCHAR*)data)[14], ((TCHAR*)data)[15]);
            result = buf;
        }
        break;

    case DEVPROP_TYPE_DATE:     //DATE is OLE Automation date. 1899/12/30 0:0:0 ==> 0.00, 1900/01/01 0:0:0 is 2.00
        {
            //todo: implement this convertion
        }
        break;
    case DEVPROP_TYPE_FILETIME:
        {
            SYSTEMTIME systime = {0};
            FileTimeToSystemTime((FILETIME*)data, &systime);
            TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
            _stprintf_s(buf, SMALL_BUFFER_SIZE, FORMAT_SYSTEMTIME,
                    systime.wYear, systime.wMonth, systime.wDay,
                    systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds);
            result = buf;
        }
        break;

    case DEVPROP_TYPE_STRING:
        {
            result = (TCHAR*) data;
        }
        break;

    case DEVPROP_TYPE_STRING_LIST:
        {
            TCHAR *ptr = (TCHAR*)data;
            while(ptr[0] != _T('\0'))
            {
                size_t len = _tcslen(ptr);
                if(0 == len)
                    break;
                if(result.size() > 0)
                    result += _T(",");
                result += ptr;
                ptr += (len+1);
            }
        }
        break;
    }

}

__inline void GetDevicePropertry(HDEVINFO infoset, PSP_DEVINFO_DATA infodata, 
                    CONST DEVPROPKEY* key, DEVPROPTYPE type, tstring &result)
{
    BYTE buffer[BIG_BUFFER_SIZE] = { 0 };
    BOOL ok = SetupDiGetDeviceProperty(infoset, infodata, key, &type, buffer, BIG_BUFFER_SIZE, NULL, 0);
    if (!ok)
    {
        result = _T("");
        return;
    }
    ParsePropertyToString(buffer, type, result);
}

size_t EnumerateDevices(list<DEVICE_PROPERTY_INFO> &result)
{
    HDEVINFO infoset;
    infoset = SetupDiGetClassDevs(
        NULL,
        _T("ROOT"),
        NULL,
        DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if(INVALID_HANDLE_VALUE != infoset)
    {
        DWORD id = 0;
        SP_DEVINFO_DATA infodata = {0};
        infodata.cbSize = sizeof(SP_DEVINFO_DATA);
        while (SetupDiEnumDeviceInfo(infoset, id++, &infodata))
        {
            DEVICE_PROPERTY_INFO data;
            BYTE buffer[BIG_BUFFER_SIZE] = {0};
            DWORD need_size = 0;
            DEVPROPTYPE type = 0;
            
            //In old windows, we should use CfgMgr API to retrieve these information from registry.
            //Since VISTA, all information of device and driver are unified to [Unified Device Property Model].
            //https://docs.microsoft.com/en-us/windows-hardware/drivers/install/unified-device-property-model--windows-vista-and-later-
            type = DEVPROP_TYPE_STRING_LIST;
            GetDevicePropertry(infoset, &infodata, &DEVPKEY_Device_HardwareIds, type, data.HardwareId);

            type = DEVPROP_TYPE_STRING;
            GetDevicePropertry(infoset, &infodata, &DEVPKEY_Device_DriverInfPath, type, data.InfName);

            type = DEVPROP_TYPE_STRING;
            GetDevicePropertry(infoset, &infodata, &DEVPKEY_Device_DriverVersion, type, data.DriverVersion);

            type = DEVPROP_TYPE_STRING;
            GetDevicePropertry(infoset, &infodata, &DEVPKEY_Device_DeviceDesc, type, data.DeviceDescription);

            type = DEVPROP_TYPE_STRING;
            GetDevicePropertry(infoset, &infodata, &DEVPKEY_Device_DriverDesc, type, data.DriverDescription);

            result.push_back(data);
        }
    }
    else
        _tprintf(_T("SetupDiGetClassDevs() failed, LastError=%d\n"), GetLastError());

    return result.size();
}

void Usage()
{}

int _tmain(int argc, _TCHAR* argv[])
{
    if(argc < 2)
    {
        Usage();
        return -1;
    }
    TCHAR *target_hwid = argv[1];

    list<DEVICE_PROPERTY_INFO> result;
    size_t count = EnumerateDevices(result);

    if(count > 0)
    {
        for(auto &item : result)
        {
            _tprintf(_T("Found device:\n"));
            _tprintf(_T("\t%s:\n"), item.DeviceDescription.c_str());
            _tprintf(_T("\t%s:\n"), item.DriverDescription.c_str());
            _tprintf(_T("\t%s:\n"), item.HardwareId.c_str());
            _tprintf(_T("\t%s:\n"), item.DriverVersion.c_str());
            _tprintf(_T("\t%s:\n"), item.InfName.c_str());
        }
    }
}
