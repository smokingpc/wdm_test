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

DEFINE_GUID (GUID_DEVINTERFACE_EventLogTest,
    0x7beffd08,0x30b7,0x402d,0x8f,0xad,0xf1,0xd4,0xe5,0x12,0x54,0x7f);
// {7beffd08-30b7-402d-8fad-f1d4e512547f}
