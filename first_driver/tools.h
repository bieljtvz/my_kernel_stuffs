#pragma once
#include <ntifs.h>
#include <windef.h>
#include <ntstrsafe.h>
#include <ntddk.h>

#include "structs.h"

//
// Define log for Drive output of DebugView
#define log( format, ... ) DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[Project] " format "\n", ##__VA_ARGS__ )





namespace tools
{

	NTSTATUS KeReadVirtualMemory(HANDLE PID, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size);
	NTSTATUS KeWriteVirtualMemory(HANDLE PID, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size);
	NTSTATUS GetProcessImageName(HANDLE processId, PUNICODE_STRING ProcessImageName);
	PVOID UtlpGetModuleBaseWow64(_In_ PEPROCESS Process, _In_ PUNICODE_STRING ModuleName);
	PVOID UtlpGetModuleBaseNative(_In_ PEPROCESS Process, _In_ PUNICODE_STRING ModuleName);
	NTSTATUS FindProcess(_In_ HANDLE ProcessId, _Out_ PEPROCESS* Process, _Out_ PBOOLEAN IsWow64);
	PVOID UtlGetLdrLoadDll(_In_ PEPROCESS Process, _In_ BOOLEAN IsWow64);
	DWORD_PTR get_module_handle_native(HANDLE pid, LPCWSTR module_name);
	DWORD get_module_handle_x32(HANDLE pid, LPCWSTR module_name);
	PVOID UtlGetModuleBase(_In_ PEPROCESS Process, _In_ PUNICODE_STRING ModuleName, _In_ BOOLEAN IsWow64);

};

