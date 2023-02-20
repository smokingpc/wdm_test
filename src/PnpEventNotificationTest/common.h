#pragma once
#include <wdm.h>
#include <initguid.h>
#include <ntddstor.h>

#define MYTAG 'YOR'

DRIVER_INITIALIZE   DriverEntry;
DRIVER_UNLOAD       DriverUnload;

DRIVER_NOTIFICATION_CALLBACK_ROUTINE NotifyCallback;
//KSTART_ROUTINE      MyThreadFunction;