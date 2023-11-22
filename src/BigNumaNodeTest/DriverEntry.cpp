#include <wdm.h>

#define TAG_GENERIC_BUFFER  ((ULONG)'FUBG')

EXTERN_C_START
DRIVER_INITIALIZE DriverEntry;

void DetectBigNumaNode()
{
    USHORT active_nodes = KeQueryActiveGroupCount();
    USHORT max_logical_node = KeQueryHighestNodeNumber();
    KdPrintEx((DPFLTR_DEFAULT_ID, 0,"[Roy Debug] Max Logical Nodes=%d, Active Logical Nodes=%d\n", 
        max_logical_node, active_nodes));

    for(USHORT i=0; i<max_logical_node; i++)
    { 
        //API supported since Win2022.
        //ULONG active_cpus = KeQueryNodeActiveProcessorCount(i);
        ULONG active_cpus = KeQueryActiveProcessorCountEx(i);
        ULONG max_cpus = KeQueryMaximumProcessorCountEx(i);
        KdPrintEx((DPFLTR_DEFAULT_ID, 0,"[Roy Debug] Logical Nodes[%d] Max CPUs=%d, Active CPUs=%d\n", 
            i, max_cpus, active_cpus));

        USHORT cpu_count = 0;
        KeQueryNodeActiveAffinity(i, NULL, &cpu_count);
        KdPrintEx((DPFLTR_DEFAULT_ID, 0,"[Roy Debug] node[%d] has %d cpus.\n", i, cpu_count));
    }

    ULONG size = PAGE_SIZE * 128;
//    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)
    PUCHAR buffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, size, TAG_GENERIC_BUFFER);

    RtlZeroMemory(buffer, size);
    NTSTATUS status = 
        KeQueryLogicalProcessorRelationship(NULL, RelationNumaNode, 
            (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer, &size);
    if(NT_SUCCESS(status))
    {
        ULONG offset = 0;
        while (size > 0)
        {
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buffer+offset);
            KdPrintEx((DPFLTR_DEFAULT_ID, 0,"[Roy Debug] NumaNode Relation=> "));
            KdPrintEx((DPFLTR_DEFAULT_ID, 0,"Node(%d) GroupCount(%d) ",
                info->NumaNode.NodeNumber, info->NumaNode.GroupCount));
            if (0 == info->NumaNode.GroupCount)
            {
                KdPrintEx((DPFLTR_DEFAULT_ID, 0,"\n\tGroup=%d, Mask=%llX",
                    info->NumaNode.GroupMask.Group, info->NumaNode.GroupMask.Mask));
            }
            else
            {
                for (ULONG j = 0; j < info->NumaNode.GroupCount; j++)
                {
                    KdPrintEx((DPFLTR_DEFAULT_ID, 0,"\n\tGroup=%d, Mask=%llX",
                        info->NumaNode.GroupMasks[j].Group, info->NumaNode.GroupMasks[j].Mask));
                }
            }
            KdPrintEx((DPFLTR_DEFAULT_ID, 0,"\n"));
            offset += info->Size;
            size -= info->Size;
        }
    }
    else
    {
        KdPrintEx((DPFLTR_DEFAULT_ID, 0,"[Roy Debug] KeQueryLogicalProcessorRelationship failed. status = %08X\n", status));
    }
    ExFreePoolWithTag(buffer, TAG_GENERIC_BUFFER);
}

NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    KdPrintEx((DPFLTR_DEFAULT_ID, 0,"DriverEntry!\n"));
    DetectBigNumaNode();
    return STATUS_UNSUCCESSFUL;
}
EXTERN_C_END
