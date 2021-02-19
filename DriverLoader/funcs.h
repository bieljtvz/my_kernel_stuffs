#pragma once


struct Vec3
{
	float X, Y, Z;
};

struct WorldToScreenMatrix_t
{
	float flMatrix[16];
	float operator[](int i) const { return flMatrix[i]; }
	float& operator[](int i) { return flMatrix[i]; }
}WorldToScreenMatrix;

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

		//while(1)
		if (GetAsyncKeyState(VK_F9) & 1)
		{
			BYTE new_data[] = {0x8B,0xFF,0x55,0x8b,0xEC};
			int valor = 10;
			WriteVirtualMemory(0x1930, 0x769F0BD0, &new_data, sizeof(new_data));
			
		}

		if (GetAsyncKeyState(VK_F8) & 1)
		{
			getstatus();
		}

		Sleep(1);
	}

	CloseHandle(hDriver);

}
