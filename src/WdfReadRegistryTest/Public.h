/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_WdfReadRegistryTest,
    0x7b866f11,0x68c0,0x48d1,0x94,0x8f,0xb2,0x3e,0xdf,0xb8,0x52,0x38);
// {7b866f11-68c0-48d1-948f-b23edfb85238}
