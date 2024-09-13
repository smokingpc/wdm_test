#pragma once
NTSTATUS RefObjectByPtr(
    _In_ PVOID Object,
    _In_ ULONG HandleAttributes,
    _In_opt_ PACCESS_STATE PassedAccessState,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_TYPE ObjectType,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PHANDLE Handle);
BOOLEAN LoadUndocumentKernelAPI(PVOID* fptr, PWCHAR api_name);
