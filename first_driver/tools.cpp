#include "tools.h"

//
//Defs
typedef NTSTATUS(*QUERY_INFO_PROCESS) (__in HANDLE ProcessHandle, __in PROCESSINFOCLASS ProcessInformationClass, __out_bcount(ProcessInformationLength) PVOID ProcessInformation, __in ULONG ProcessInformationLength, __out_opt PULONG ReturnLength);
QUERY_INFO_PROCESS ZwQueryInformationProcess;

//
//Codes


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

NTSTATUS tools::GetProcessImageName(HANDLE processId, PUNICODE_STRING ProcessImageName)
{
	NTSTATUS status;
	ULONG returnedLength;
	ULONG bufferLength;
	HANDLE hProcess = NULL;
	PVOID buffer;
	PEPROCESS eProcess;
	PUNICODE_STRING imageName;

	PAGED_CODE(); // this eliminates the possibility of the IDLE Thread/Process

	status = PsLookupProcessByProcessId(processId, &eProcess);

	if (NT_SUCCESS(status))
	{
		status = ObOpenObjectByPointer(eProcess, 0, NULL, 0, 0, KernelMode, &hProcess);
		if (NT_SUCCESS(status))
		{
		}
		else 
		{
			log("ObOpenObjectByPointer Failed: %08x\n", status);
		}
		ObDereferenceObject(eProcess);
	}
	else {
		log("PsLookupProcessByProcessId Failed: %08x\n", status);
	}


	if (NULL == ZwQueryInformationProcess) {

		UNICODE_STRING routineName;

		RtlInitUnicodeString(&routineName, L"ZwQueryInformationProcess");

		ZwQueryInformationProcess =	(QUERY_INFO_PROCESS)MmGetSystemRoutineAddress(&routineName);

		if (NULL == ZwQueryInformationProcess) 
		{
			DbgPrint("Cannot resolve ZwQueryInformationProcess\n");
		}
	}

	/* Query the actual size of the process path */
	status = ZwQueryInformationProcess(hProcess,ProcessImageFileName, NULL,	0, 	&returnedLength);

	if (STATUS_INFO_LENGTH_MISMATCH != status) {
		return status;
	}

	/* Check there is enough space to store the actual process
	   path when it is found. If not return an error with the
	   required size */
	bufferLength = returnedLength - sizeof(UNICODE_STRING);
	if (ProcessImageName->MaximumLength < bufferLength)
	{
		ProcessImageName->MaximumLength = (USHORT)bufferLength;
		return STATUS_BUFFER_OVERFLOW;
	}

	/* Allocate a temporary buffer to store the path name */
	buffer = ExAllocatePoolWithTag(NonPagedPool, returnedLength, 'uLT1');

	if (NULL == buffer)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	/* Retrieve the process path from the handle to the process */
	status = ZwQueryInformationProcess(hProcess,
		ProcessImageFileName,
		buffer,
		returnedLength,
		&returnedLength);

	if (NT_SUCCESS(status))
	{
		/* Copy the path name */
		imageName = (PUNICODE_STRING)buffer;
		RtlCopyUnicodeString(ProcessImageName, imageName);
	}

	/* Free the temp buffer which stored the path */
	ExFreePoolWithTag(buffer, 'uLT1');

	return status;
}

NTSTATUS tools::FindProcess(_In_ HANDLE ProcessId, _Out_ PEPROCESS* Process, _Out_ PBOOLEAN IsWow64)
{
	NTSTATUS Status = PsLookupProcessByProcessId(ProcessId, Process);
	if (!NT_SUCCESS(Status) || *Process == NULL)
	{
		return STATUS_NOT_FOUND;
	}

	LARGE_INTEGER ZeroTime = { 0 };
	if (KeWaitForSingleObject(*Process, Executive, KernelMode, FALSE, &ZeroTime) == STATUS_WAIT_0)
	{
		// Process is terminating.
		ObDereferenceObject(*Process);
		return STATUS_PROCESS_IS_TERMINATING;
	}

	*IsWow64 = PsGetProcessWow64Process(*Process) != NULL;

	return Status;
}

 PVOID UtlpRvaToVa(	_In_ PVOID Module,	_In_ ULONG Rva)
{
	if (Rva == 0)
	{
		return NULL;
	}

	return (PVOID)((PUCHAR)Module + Rva);
}

 PVOID tools::UtlpGetModuleBaseWow64(_In_ PEPROCESS Process,	_In_ PUNICODE_STRING ModuleName, PDWORD Size)
{
	
	PPEB32 Peb = (PPEB32)PsGetProcessWow64Process(Process);
	

	if (Peb == NULL || Peb->Ldr == 0)
	{
		
		return NULL;
	}
	
	for (PLIST_ENTRY32 Entry = (PLIST_ENTRY32)((PPEB_LDR_DATA32)Peb->Ldr)->InLoadOrderModuleList.Flink;	Entry != &((PPEB_LDR_DATA32)Peb->Ldr)->InLoadOrderModuleList; Entry = (PLIST_ENTRY32)Entry->Flink)
	{
		PLDR_DATA_TABLE_ENTRY32 LdrEntry = CONTAINING_RECORD(Entry,	LDR_DATA_TABLE_ENTRY32,	InLoadOrderLinks);
		
		if (LdrEntry->BaseDllName.Buffer == 0)
		{
			continue;
		}

		UNICODE_STRING CurrentName = { 0 };
		RtlUnicodeStringInit(&CurrentName, (PWCHAR)LdrEntry->BaseDllName.Buffer);

		if (RtlEqualUnicodeString(ModuleName, &CurrentName, TRUE))
		{
			*Size = (DWORD)LdrEntry->SizeOfImage;
			return (PVOID)LdrEntry->DllBase;			
		}
	}

	
	return NULL;
}

 PVOID tools::UtlpGetModuleBaseNative(_In_ PEPROCESS Process, _In_ PUNICODE_STRING ModuleName, PDWORD Size)
{
	
	PPEB Peb = PsGetProcessPeb(Process);
	if (Peb == NULL || Peb->Ldr == NULL)
	{
		
		return NULL;
	}
	
	for (PLIST_ENTRY Entry = Peb->Ldr->InLoadOrderModuleList.Flink;		Entry != &Peb->Ldr->InLoadOrderModuleList;		Entry = Entry->Flink)
	{
		PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(	Entry,LDR_DATA_TABLE_ENTRY,InLoadOrderLinks);

		if (LdrEntry->BaseDllName.Buffer == NULL)
		{
			continue;
		}

		if (RtlEqualUnicodeString(ModuleName, &LdrEntry->BaseDllName, TRUE))
		{
			*Size = (DWORD)LdrEntry->SizeOfImage;
			return (PVOID)LdrEntry->DllBase;
			
		}
	}

	return NULL;
}

 PVOID UtlpGetModuleExport(_In_ PVOID Module,	_In_ PCHAR ExportName)
{
	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Module;
	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		return NULL;
	}

	PIMAGE_NT_HEADERS32 NtHeaders32 = (PIMAGE_NT_HEADERS32)UtlpRvaToVa(Module, DosHeader->e_lfanew);
	PIMAGE_NT_HEADERS64 NtHeaders64 = (PIMAGE_NT_HEADERS64)NtHeaders32;
	if (NtHeaders64 == NULL || NtHeaders64->Signature != IMAGE_NT_SIGNATURE)
	{
		return NULL;
	}

	PIMAGE_DATA_DIRECTORY DataDirectory = NULL;
	if (NtHeaders64->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		DataDirectory = &NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	}
	else if (NtHeaders64->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		DataDirectory = &NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	}
	else
	{
		return NULL;
	}

	ULONG ExportDirectorySize = DataDirectory->Size;
	PIMAGE_EXPORT_DIRECTORY ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)UtlpRvaToVa(Module, DataDirectory->VirtualAddress);
	if (ExportDirectory == NULL)
	{
		return NULL;
	}

	PULONG Names = (PULONG)UtlpRvaToVa(Module, ExportDirectory->AddressOfNames);
	PULONG Funcs = (PULONG)UtlpRvaToVa(Module, ExportDirectory->AddressOfFunctions);
	PUSHORT Ords = (PUSHORT)UtlpRvaToVa(Module, ExportDirectory->AddressOfNameOrdinals);
	if (Names == NULL || Funcs == NULL || Ords == NULL)
	{
		return NULL;
	}

	for (ULONG Index = 0; Index < ExportDirectory->NumberOfNames; ++Index)
	{
		PCHAR CurrentName = (PCHAR)UtlpRvaToVa(Module, Names[Index]);

		if (CurrentName != NULL && strncmp(ExportName, CurrentName, 256) == 0)
		{
			USHORT CurrentOrd = Ords[Index];

			if (CurrentOrd < ExportDirectory->NumberOfFunctions)
			{
				PVOID ExportAddress = UtlpRvaToVa(Module, Funcs[CurrentOrd]);

				// Export is forwarded.
				if ((ULONG_PTR)ExportAddress >= (ULONG_PTR)ExportDirectory &&
					(ULONG_PTR)ExportAddress <= (ULONG_PTR)ExportDirectory + ExportDirectorySize)
				{
					return NULL;
				}

				return ExportAddress;
			}

			return NULL;
		}
	}

	return NULL;
}

PVOID UtlGetModuleExport(_In_ PVOID Module,	_In_ PCHAR ExportName)
{
	__try
	{
		return UtlpGetModuleExport(Module, ExportName);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return NULL;
	}
}

PVOID tools::UtlGetModuleBase(_In_ PEPROCESS Process,	_In_ PUNICODE_STRING ModuleName,	_In_ BOOLEAN IsWow64, PDWORD Size)
{
	
	__try
	{
		if (IsWow64)
		{
			return tools::UtlpGetModuleBaseWow64(Process, ModuleName, Size);
		}
		else
		{		
			return tools::UtlpGetModuleBaseNative(Process, ModuleName, Size);
		}

		
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		log("UtlGetModuleBase erro: ");
		return NULL;
	}
}

//PVOID tools::UtlGetLdrLoadDll(_In_ PEPROCESS Process/*, _In_ BOOLEAN IsWow64*/)
//{
//	UNICODE_STRING NtdllString = RTL_CONSTANT_STRING(L"ntdll.dll");
//	PVOID Ntdll = UtlGetModuleBase(Process, &NtdllString/*, IsWow64*/);
//	log("NTDLL: %p", Ntdll);
//	if (Ntdll == NULL)
//	{
//		return NULL;
//	}
//
//	return UtlGetModuleExport(Ntdll, "LdrLoadDll");
//}