#pragma once
HANDLE hDriver; // Handle to driver

// Request to read virtual user memory (memory of a program) from kernel space
#define IO_READ_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0701 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to write virtual user memory (memory of a program) from kernel space
#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0702 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

typedef struct _KERNEL_READ_REQUEST
{
	ULONG ProcessId;

	ULONG Address;
	ULONG Response;
	ULONG Size;

} KERNEL_READ_REQUEST, * PKERNEL_READ_REQUEST;

typedef struct _KERNEL_WRITE_REQUEST
{
	ULONG ProcessId;

	ULONG Address;
	ULONG Value;
	ULONG Size;

} KERNEL_WRITE_REQUEST, * PKERNEL_WRITE_REQUEST;

template <typename type>
type ReadVirtualMemory(ULONG ProcessId, ULONG ReadAddress, SIZE_T Size)
{
	if (hDriver == INVALID_HANDLE_VALUE)
		return (type)false;

	DWORD Return, Bytes;
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

bool WriteVirtualMemory(ULONG ProcessId, ULONG WriteAddress, ULONG WriteValue, SIZE_T WriteSize)
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
