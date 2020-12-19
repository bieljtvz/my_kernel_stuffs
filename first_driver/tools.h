#pragma once

#include <ntifs.h>
#include <windef.h>

#include "structs.h"

//
// Define log for Drive output of DebugView
#define log( format, ... ) DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[Project] " format "\n", ##__VA_ARGS__ )


namespace tools
{

	NTSTATUS KeReadVirtualMemory(HANDLE PID, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size);
	NTSTATUS KeWriteVirtualMemory(HANDLE PID, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size);

};