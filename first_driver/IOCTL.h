#pragma once

#define IO_PROTECT_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x700, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IO_READ_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0701, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0702, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IO_GETMODULE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0703, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IO_ALLOC_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0704, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IO_CREATETHREAD_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x705, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IO_FREEMEMORY_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x706, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

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
	//Adpatar IoIs32bitProcess 
	ULONG ControlCode = stack->Parameters.DeviceIoControl.IoControlCode;

	if (ControlCode == IO_READ_REQUEST)
	{	
		memcpy(&UserLand, Irp->AssociatedIrp.SystemBuffer, sizeof(UserLand));	

		UserBuffer = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

		if (UserBuffer && UserLand.Addr != NULL && (DWORD_PTR)UserLand.Addr < 0x7fffffffffff)
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
		if (UserBuffer && UserLand.Addr != NULL && (DWORD_PTR)UserLand.Addr < 0x7fffffffffff)
		{		

			//ULONG Protection = 0;
			//tools::VirtualProtectKM((HANDLE)UserLand.PID, UserLand.Addr, UserLand.Bytes, PAGE_EXECUTE_READWRITE, &Protection);
			tools::KeWriteVirtualMemory((HANDLE)UserLand.PID, UserLand.Addr, (PVOID)UserBuffer, UserLand.Bytes);
			//tools::VirtualProtectKM((HANDLE)UserLand.PID, UserLand.Addr, UserLand.Bytes, PAGE_EXECUTE_READ, &Protection);
			//tools::writeToReadOnly((HANDLE)UserLand.PID, UserLand.Addr, (PVOID)UserBuffer, UserLand.Bytes, 0);
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
	else if (ControlCode == IO_ALLOC_REQUEST)
	{
		PKERNEL_ALLOC_REQUEST VirtualAllocInput = (PKERNEL_ALLOC_REQUEST)Irp->AssociatedIrp.SystemBuffer;		
		
		PVOID Allocated = (PVOID)tools::VirtualAllocKM((HANDLE)VirtualAllocInput->ProcessId, VirtualAllocInput->AllocationType, VirtualAllocInput->Protection, VirtualAllocInput->Size);
		VirtualAllocInput->Response = (PVOID)Allocated;		
		log("VirtualAllocInput->AllocationType: %p", VirtualAllocInput->AllocationType);
		log("VirtualAllocInput->Protection: %p", VirtualAllocInput->Protection);
		log("VirtualAllocInput->Size: %p", VirtualAllocInput->Size);

		Status = STATUS_SUCCESS;
		BytesIO = sizeof(KERNEL_ALLOC_REQUEST);
		
	}
	else if (ControlCode == IO_PROTECT_REQUEST)
	{
		
		PKERNEL_PROTECT_REQUEST ProtectInput = (PKERNEL_PROTECT_REQUEST)Irp->AssociatedIrp.SystemBuffer;

		ULONG OldProtect;
		tools::VirtualProtectKM((HANDLE)ProtectInput->ProcessId, (PVOID)ProtectInput->Address, ProtectInput->Size, ProtectInput->NewProtect, &OldProtect);

		log("OldProtect: %X", OldProtect);
		ProtectInput->OldProtect = OldProtect;

		Status = STATUS_SUCCESS;
		BytesIO = sizeof(KERNEL_PROTECT_REQUEST);
	}
	else if (ControlCode == IO_CREATETHREAD_REQUEST)
	{
		PKERNEL_CREATETHREAD_REQUEST CreateThreadInput = (PKERNEL_CREATETHREAD_REQUEST)Irp->AssociatedIrp.SystemBuffer;

		auto status = tools::CreateThreadKM((HANDLE)CreateThreadInput->ProcessId, (PVOID)CreateThreadInput->StartAddress, CreateThreadInput->StartParam);
		log("Thread Status: %p", status);		

		Status = STATUS_SUCCESS;
		BytesIO = sizeof(KERNEL_CREATETHREAD_REQUEST);

	}
	else if (ControlCode == IO_FREEMEMORY_REQUEST)
	{
		PKERNEL_FREEMEMORY_REQUEST FreeMemoryInput = (PKERNEL_FREEMEMORY_REQUEST)Irp->AssociatedIrp.SystemBuffer;

		auto status = tools::FreeVirtualMemory((HANDLE)FreeMemoryInput->ProcessId, FreeMemoryInput->Address, FreeMemoryInput->Size, FreeMemoryInput->Type);
		log("FreeMemory Status: %p", status);

		Status = STATUS_SUCCESS;
		BytesIO = sizeof(KERNEL_FREEMEMORY_REQUEST);
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