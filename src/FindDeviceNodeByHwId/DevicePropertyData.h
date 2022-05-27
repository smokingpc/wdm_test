#pragma once

typedef std::basic_string<TCHAR> tstring;
typedef struct _DEVICE_PROPERTY_INFO
{
    tstring HardwareId = _T("");
    tstring InfName = _T("");           // "inf name" in DeviceManager UI
    tstring DriverVersion = _T("");
    tstring DeviceDescription = _T("");
    tstring DriverDescription = _T("");
}DEVICE_PROPERTY_INFO, *PDEVICE_PROPERTY_INFO;
