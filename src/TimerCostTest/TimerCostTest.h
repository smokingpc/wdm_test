#pragma once
#include <wdm.h>
#include <ntstrsafe.h>

DRIVER_INITIALIZE   DriverEntry;
DRIVER_UNLOAD       DriverUnload;

KDEFERRED_ROUTINE DummyDpcRoutine;
KDEFERRED_ROUTINE TimerDpcRoutine;
KTIMER MyTimer;


#define TAG_DPC_ARRAY   ((ULONG) 'SCPD')
