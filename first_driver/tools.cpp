#include "tools.h"

//NTSTATUS KeReadVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size)
//{
//	SIZE_T Bytes;
//	if (NT_SUCCESS(MmCopyVirtualMemory(Process, SourceAddress, PsGetCurrentProcess(),TargetAddress, Size, KernelMode, &Bytes)))
//		return STATUS_SUCCESS;
//	else
//		return STATUS_ACCESS_DENIED;
//}
//
//NTSTATUS KeWriteVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size)
//{
//	SIZE_T Bytes;
//	if (NT_SUCCESS(MmCopyVirtualMemory(PsGetCurrentProcess(), SourceAddress, Process,TargetAddress, Size, KernelMode, &Bytes)))
//		return STATUS_SUCCESS;
//	else
//		return STATUS_ACCESS_DENIED;
//}

NTSTATUS tools::KeReadVirtualMemory(HANDLE PID, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size)
{
    SIZE_T Result;
    PEPROCESS SourceProcess, TargetProcess;
    PsLookupProcessByProcessId(PID, &SourceProcess);   

    TargetProcess = PsGetCurrentProcess();
    __try 
    {
        
        MmCopyVirtualMemory(SourceProcess, SourceAddress, TargetProcess, TargetAddress, Size, KernelMode, &Result);
        return STATUS_SUCCESS;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return STATUS_ACCESS_DENIED;
    }
}

NTSTATUS tools::KeWriteVirtualMemory(HANDLE PID, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size)
{
    SIZE_T Result;
    PEPROCESS SourceProcess, TargetProcess;
    PsLookupProcessByProcessId(PID, &SourceProcess);
    TargetProcess = PsGetCurrentProcess();
    __try 
    {   
        MmCopyVirtualMemory(TargetProcess, SourceAddress, SourceProcess, TargetAddress, Size, KernelMode, &Result);
        return STATUS_SUCCESS;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return STATUS_ACCESS_DENIED;
    }
}
