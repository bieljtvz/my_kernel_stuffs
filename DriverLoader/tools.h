#pragma once
HANDLE hDriver; // Handle to driver

// Request to read virtual user memory (memory of a program) from kernel space
#define IO_READ_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0701 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to write virtual user memory (memory of a program) from kernel space
#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0702 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#define IO_GETMODULEBASE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0703 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

typedef struct _KERNEL_READ_REQUEST
{
	ULONG ProcessId;
	DWORD_PTR  Address;
	DWORD_PTR  Response;
	ULONG Size;

} KERNEL_READ_REQUEST, * PKERNEL_READ_REQUEST;

typedef struct _KERNEL_WRITE_REQUEST
{
	ULONG ProcessId;
	DWORD_PTR Address;
	ULONG Value;
	ULONG Size;

} KERNEL_WRITE_REQUEST, * PKERNEL_WRITE_REQUEST;

typedef struct _KERNEL_GETMODULEBASE_REQUEST
{
	ULONG ProcessId;
	WCHAR name[260];
	bool isWow64;
	DWORD_PTR BaseAddress;

} KERNEL_GETMODULEBASE_REQUEST, * PKERNEL_GETMODULEBASE_REQUEST;

template <typename type>
type ReadVirtualMemory(ULONG ProcessId, DWORD_PTR ReadAddress, SIZE_T Size)
{
	if (hDriver == INVALID_HANDLE_VALUE)
		return (type)false;

	DWORD_PTR Return, Bytes;
	KERNEL_READ_REQUEST ReadRequest;

	ReadRequest.ProcessId = ProcessId;
	ReadRequest.Address = ReadAddress;
	ReadRequest.Size = Size;

	// send code to our driver with the arguments
	if (DeviceIoControl(hDriver, IO_READ_REQUEST, &ReadRequest, sizeof(ReadRequest), &ReadRequest, sizeof(ReadRequest), 0, 0))
		return (type)ReadRequest.Response;
	else
		return (type)false;
}

bool WriteVirtualMemory(ULONG ProcessId, DWORD_PTR WriteAddress, ULONG WriteValue, SIZE_T WriteSize)
{
	if (hDriver == INVALID_HANDLE_VALUE)
		return false;
	DWORD Bytes;

	KERNEL_WRITE_REQUEST  WriteRequest;
	WriteRequest.ProcessId = ProcessId;
	WriteRequest.Address = WriteAddress;
	WriteRequest.Value = WriteValue;
	WriteRequest.Size = WriteSize;

	if (DeviceIoControl(hDriver, IO_WRITE_REQUEST, &WriteRequest, sizeof(WriteRequest), 0, 0, &Bytes, NULL))
		return true;
	else
		return false;
}

bool ModuleBase(ULONG ProcessId, const std::string& module_name)
{
	if (hDriver == INVALID_HANDLE_VALUE)
		return 0;

	DWORD Bytes;

	KERNEL_GETMODULEBASE_REQUEST  GetModuleBaseRequest;
	GetModuleBaseRequest.ProcessId = ProcessId;
	GetModuleBaseRequest.isWow64 = 1;
	
	std::wstring wstr{ std::wstring(module_name.begin(), module_name.end()) };
	memset(GetModuleBaseRequest.name, 0, sizeof(WCHAR) * 260);
	wcscpy(GetModuleBaseRequest.name, wstr.c_str());
	
	GetModuleBaseRequest.BaseAddress = NULL;
	

	if (DeviceIoControl(hDriver, IO_GETMODULEBASE_REQUEST, &GetModuleBaseRequest, sizeof(GetModuleBaseRequest), 0, 0, &Bytes, NULL))
	{
		return true;
	}
	else
		return false;

}
