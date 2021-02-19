#pragma once

HANDLE hDriver; 

#define IO_READ_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0701, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0702, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define IO_GETMODULEBASE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0703, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#define IO_GET_STATUS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0703, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

//Init Structs//


typedef struct _KERNEL_GETMODULEBASE_REQUEST
{
	ULONG ProcessId;
	WCHAR name[260];
	DWORD_PTR BaseAddress;
	DWORD Size;
} KERNEL_GETMODULEBASE_REQUEST, * PKERNEL_GETMODULEBASE_REQUEST;

//End of structs//


//
// Some Functions
bool ModuleBaseInfo(ULONG ProcessId, const std::string& module_name, PDWORD_PTR BaseAddress, PDWORD Size)
{
	if (hDriver == INVALID_HANDLE_VALUE)
		return 0;

	KERNEL_GETMODULEBASE_REQUEST  GetModuleBaseRequest;
	GetModuleBaseRequest.ProcessId = ProcessId;

	std::wstring wstr{ std::wstring(module_name.begin(), module_name.end()) };
	memset(GetModuleBaseRequest.name, 0, sizeof(WCHAR) * 260);
	wcscpy(GetModuleBaseRequest.name, wstr.c_str());

	if (DeviceIoControl(hDriver, IO_GETMODULEBASE_REQUEST, &GetModuleBaseRequest, sizeof(GetModuleBaseRequest), &GetModuleBaseRequest, sizeof(GetModuleBaseRequest), 0, 0))
	{
		*BaseAddress = GetModuleBaseRequest.BaseAddress;
		*Size = GetModuleBaseRequest.Size;
		return true;
	}
	else
		return false;

}

void WriteVirtualMemory(ULONG ProcessId, DWORD_PTR WriteAddress, LPVOID WriteValue, SIZE_T WriteSize)
{
	if (hDriver == INVALID_HANDLE_VALUE)
		return;

	struct Rpmdata
	{
		HANDLE pid;
		PVOID SourceAddress;
		PVOID TargetAddress;
		SIZE_T Size;
	} rpm;
	rpm.pid = (HANDLE)ProcessId;
	rpm.SourceAddress = (PVOID)WriteAddress;
	rpm.TargetAddress = WriteValue;
	rpm.Size = WriteSize;

	//HANDLE hDevice = INVALID_HANDLE_VALUE;
	BOOL bResult = FALSE;
	DWORD junk = 0;

	if (DeviceIoControl(hDriver, IO_WRITE_REQUEST, &rpm, sizeof(rpm), WriteValue, WriteSize, &junk, (LPOVERLAPPED)NULL))	
		printf("[+] DeviceIoCointrol 1\n");			
	else
		printf("[+] DeviceIoCointrol 0\n");
}


void ReadVirtualMemory(ULONG ProcessId, DWORD_PTR ReadAddress, LPVOID lpBuffer, SIZE_T Size)
{
	if (hDriver == INVALID_HANDLE_VALUE)
		return;

	struct Rpmdata
	{
		HANDLE pid;
		PVOID SourceAddress;
		PVOID TargetAddress;
		SIZE_T Size;
	} rpm;

	rpm.pid = (HANDLE)ProcessId;
	rpm.SourceAddress = (PVOID)ReadAddress;
	rpm.TargetAddress = lpBuffer;
	rpm.Size = Size;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	BOOL bResult = FALSE;
	DWORD junk = 0;

	// send code to our driver with the arguments
	bool result = DeviceIoControl(hDriver, IO_READ_REQUEST, &rpm, sizeof(rpm), lpBuffer, Size, &junk, (LPOVERLAPPED)NULL);

}

bool getstatus()
{
	bool result = DeviceIoControl(hDriver, IO_GET_STATUS, NULL, NULL, NULL,NULL, NULL, (LPOVERLAPPED)NULL);
	return result;
}

//template <class cData>
//cData read(DWORD_PTR Address) 
//{
//	cData B;
//	SIZE_T bytesRead;
//	ReadVirtualMemory((LPCVOID)Address, &B, sizeof(B), &bytesRead);
//	return B;
//}

