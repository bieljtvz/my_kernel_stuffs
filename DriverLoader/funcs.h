#pragma once


void init_control()
{
	//
	// Get handle from our Device created in our driver
	hDriver = CreateFileA("\\\\.\\first_driver", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hDriver == INVALID_HANDLE_VALUE)
	{
		printf("[-] Failed to create file: \n");
		return;
	}

	static int global_var = NULL;
	
	while (!(GetAsyncKeyState(VK_END) & 1))
	{

		if (GetAsyncKeyState(VK_F9) & 1)
		{
			/*DWORD find_addr = ReadVirtualMemory<DWORD>((DWORD)PID,0x005A1040, 4);
			printf("[+] Address: %p\n",find_addr);			*/

			ModuleBase(0x3770,"ucrtbase.dll");

		}

		Sleep(1);
	}

	CloseHandle(hDriver);

}
