#pragma once
// Request to read virtual user memory (memory of a program) from kernel space
#define IO_READ_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0701, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to write virtual user memory (memory of a program) from kernel space
#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0702, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#define IO_GETMODULE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0703, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

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

// IOCTL Call Handler function
NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	NTSTATUS Status;
	ULONG BytesIO = 0;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	// Code received from user space
	ULONG ControlCode = stack->Parameters.DeviceIoControl.IoControlCode;

	if (ControlCode == IO_READ_REQUEST)
	{
		// Get the input buffer & format it to our struct
		PKERNEL_READ_REQUEST ReadInput = (PKERNEL_READ_REQUEST)Irp->AssociatedIrp.SystemBuffer;
		PKERNEL_READ_REQUEST ReadOutput = (PKERNEL_READ_REQUEST)Irp->AssociatedIrp.SystemBuffer;
		

		//
		//Call our function 
		tools::KeReadVirtualMemory((HANDLE)ReadInput->ProcessId, (PVOID)ReadInput->Address, &ReadInput->Response, ReadInput->Size);		
		
		DbgPrintEx(0, 0, "Read Params:  %lu, %#010x \n", ReadInput->ProcessId, ReadInput->Address);
		DbgPrintEx(0, 0, "Value: %lu \n", ReadOutput->Response);

		Status = STATUS_SUCCESS;
		BytesIO = sizeof(KERNEL_READ_REQUEST);
	}
	else if (ControlCode == IO_WRITE_REQUEST)
	{
		// Get the input buffer & format it to our struct
		PKERNEL_WRITE_REQUEST WriteInput = (PKERNEL_WRITE_REQUEST)Irp->AssociatedIrp.SystemBuffer;

		tools::KeWriteVirtualMemory((HANDLE)WriteInput->ProcessId, &WriteInput->Value, (PVOID)WriteInput->Address, WriteInput->Size);

		Status = STATUS_SUCCESS;
		BytesIO = sizeof(KERNEL_WRITE_REQUEST);
	}	
	else if (ControlCode == IO_GETMODULE_REQUEST)
	{
		PKERNEL_GETMODULEBASE_REQUEST GetModuleBaseInput = (PKERNEL_GETMODULEBASE_REQUEST)Irp->AssociatedIrp.SystemBuffer;	
		PKERNEL_GETMODULEBASE_REQUEST GetModuleBaseOutput = (PKERNEL_GETMODULEBASE_REQUEST)Irp->AssociatedIrp.SystemBuffer;

		PEPROCESS hProcess;	
		BOOLEAN iswow;
		DWORD SizeOfImage;

		tools::FindProcess((HANDLE)GetModuleBaseInput->ProcessId, &hProcess, &iswow);

		KAPC_STATE ApcState = { 0 };
		KeStackAttachProcess(hProcess, &ApcState);

		UNICODE_STRING CurrentName = { 0 };
		
		UNICODE_STRING module_name_unicode;
		RtlInitUnicodeString(&module_name_unicode, GetModuleBaseInput->name);

		PVOID GetModuleBase = tools::UtlGetModuleBase(hProcess, &module_name_unicode, iswow, &SizeOfImage);

		log("Image: %p", GetModuleBase);
		log("SizeOfImage: %X", SizeOfImage);

		GetModuleBaseInput->BaseAddress = (DWORD_PTR)GetModuleBase;
		GetModuleBaseInput->Size = SizeOfImage;

		log("Image: %p", GetModuleBaseOutput->BaseAddress);
		
		KeUnstackDetachProcess(&ApcState);
		ObDereferenceObject(hProcess);

		Status = STATUS_SUCCESS;
		BytesIO = sizeof(PKERNEL_GETMODULEBASE_REQUEST);
	}
	else
	{
		// if the code is unknown
		Status = STATUS_INVALID_PARAMETER;
		BytesIO = 0;
	}

	// Complete the request
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = BytesIO;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}