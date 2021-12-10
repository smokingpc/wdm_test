// ListHDDInstance.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <SetupAPI.h>
#include <iostream>

int main()
{
    HDEVINFO hdev;
    DWORD idx;
    GUID guid = GUID_DEVINTERFACE_DISK;
    //CString csDevicePath;
    BOOL bRet = TRUE;
    BOOL nStatus;
    DWORD dwSize = 0;

    hdev = SetupDiGetClassDevs(&guid,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hdev == INVALID_HANDLE_VALUE)
    {
        printf("ERROR : Unable to enumerate device.\n");
        return FALSE;
    }

    SP_DEVICE_INTERFACE_DATA  DeviceInterfaceData;
    DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);

    for (idx = 0; SetupDiEnumDeviceInterfaces(hdev, NULL, &guid, idx, &DeviceInterfaceData); idx++)
    {
        nStatus = SetupDiGetDeviceInterfaceDetail(hdev, &DeviceInterfaceData, NULL, 0, &dwSize, NULL);
        if (!dwSize)
        {
            bRet = FALSE;
            printf("ERROR : SetupDiGetDeviceInterfaceDetail fial.\n");
            break;
        }

        PSP_DEVICE_INTERFACE_DETAIL_DATA pBuffer = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(dwSize);
        ZeroMemory(pBuffer, sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
        pBuffer->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        SP_DEVINFO_DATA DeviceInfoData = { sizeof(SP_DEVINFO_DATA) };
        nStatus = SetupDiGetDeviceInterfaceDetail(hdev, &DeviceInterfaceData, pBuffer, dwSize, &dwSize, &DeviceInfoData);
        if (!nStatus)
        {
            bRet = FALSE;
            printf("ERROR : SetupDiGetDeviceInterfaceDetail fial.\n");
            break;
        }

        printf("%S\n", pBuffer->DevicePath);
    }
    SetupDiDestroyDeviceInfoList(hdev);

    return 0;

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
