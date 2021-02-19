#pragma once

#define IO_READ_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0701, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0702, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IO_GETMODULE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0703, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IO_GET_STATUS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0704, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

PDEVICE_OBJECT pDeviceObject; // our driver object
UNICODE_STRING dev, dos; // Driver registry paths

NTSTATUS CreateCall(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS CloseCall(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

struct {
	int PID;
	void* Addr;
	void* Value;
	int Bytes;
}
UserLand;


// IOCTL Call Handler function
NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	PUCHAR UserBuffer;

	NTSTATUS Status;
	ULONG BytesIO = 0;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG ControlCode = stack->Parameters.DeviceIoControl.IoControlCode;

	if (ControlCode == IO_READ_REQUEST)
	{	
		memcpy(&UserLand, Irp->AssociatedIrp.SystemBuffer, sizeof(UserLand));	

		UserBuffer = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
		if (UserBuffer && UserLand.Addr != NULL) 
		{
			tools::KeReadVirtualMemory((HANDLE)UserLand.PID, UserLand.Addr, (PVOID)UserBuffer, UserLand.Bytes);
		}
		KeFlushIoBuffers(Irp->MdlAddress, TRUE, FALSE);
		Status = 0;
		BytesIO = 0;
		
	}
	else if (ControlCode == IO_WRITE_REQUEST)
	{		
		memcpy(&UserLand, Irp->AssociatedIrp.SystemBuffer, sizeof(UserLand));
		UserBuffer = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
		if (UserBuffer && UserLand.Addr != NULL)
		{		
			tools::KeWriteVirtualMemory((HANDLE)UserLand.PID, UserLand.Addr, (PVOID)UserBuffer, UserLand.Bytes);
			//tools::writeToReadOnly(UserLand.Addr, (PVOID)UserBuffer, UserLand.Bytes);
			//tools::VirtualAllocKM((HANDLE)0x1880, MEM_COMMIT| MEM_RESERVE, PAGE_EXECUTE_READWRITE, 0x8);
			//tools::VirtualProtectKM((HANDLE)0x1930, (PVOID)0x76070BD0, 8, PAGE_EXECUTE_READWRITE);
		}		
		KeFlushIoBuffers(Irp->MdlAddress, TRUE, FALSE);
		Status = 0;
		BytesIO = 0;
	}
	else if (ControlCode == IO_GETMODULE_REQUEST)
	{
		PKERNEL_GETMODULEBASE_REQUEST GetModuleBaseInput = (PKERNEL_GETMODULEBASE_REQUEST)Irp->AssociatedIrp.SystemBuffer;			

		PEPROCESS hProcess;	
		BOOLEAN IsWow64;
		
		if (NT_SUCCESS(tools::FindProcess((HANDLE)GetModuleBaseInput->ProcessId, &hProcess, &IsWow64)))
		{

			KAPC_STATE ApcState = { 0 };
			KeStackAttachProcess(hProcess, &ApcState);

			UNICODE_STRING module_name_unicode;
			RtlInitUnicodeString(&module_name_unicode, GetModuleBaseInput->name);

			PVOID GetModuleBase = tools::UtlGetModuleBase(hProcess, &module_name_unicode, IsWow64, &GetModuleBaseInput->Size);

			GetModuleBaseInput->BaseAddress = (DWORD_PTR)GetModuleBase;

			log("GetModuleBaseInput->BaseAddress: %p", GetModuleBaseInput->BaseAddress);
			log("GetModuleBaseInput->Size: %p", GetModuleBaseInput->Size);

			KeUnstackDetachProcess(&ApcState);
			ObDereferenceObject(hProcess);

			Status = STATUS_SUCCESS;
		}
		else
		{
			Status = STATUS_NOT_FOUND;
		}
		
		BytesIO = sizeof(KERNEL_GETMODULEBASE_REQUEST);
	}	


	else
	{		
		Status = STATUS_INVALID_PARAMETER;
		BytesIO = 0;
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = BytesIO;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}