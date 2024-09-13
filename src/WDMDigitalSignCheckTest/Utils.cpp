#include <ntifs.h>
#include "Utils.h"


NTSTATUS RefObjectByPtr(
    _In_ PVOID Object,
    _In_ ULONG HandleAttributes,
    _In_opt_ PACCESS_STATE PassedAccessState,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_TYPE ObjectType,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PHANDLE Handle)
{
    return ObOpenObjectByPointer(
        Object, HandleAttributes,
        PassedAccessState, DesiredAccess,
        ObjectType, AccessMode, Handle);
}

BOOLEAN LoadUndocumentKernelAPI(PVOID* fptr, PWCHAR api_name)
{
    UNICODE_STRING name = RTL_CONSTANT_STRING(api_name);
    *fptr = MmGetSystemRoutineAddress(&name);

    if (nullptr == *fptr)
    {
        KdPrint(("Cannot resolve %S\n", api_name));
        return FALSE;
    }

    return TRUE;
}
