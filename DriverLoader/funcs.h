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
		//if (GetAsyncKeyState(VK_F9) & 1)
		{
			
			
			ReadVirtualMemory(0x24AC, 0x04930000 + 0xE956A0, &WorldToScreenMatrix, sizeof(WorldToScreenMatrix_t));
			printf("FloatZ: %f\n", WorldToScreenMatrix.flMatrix[3]);
			//system("cls");
		}

		Sleep(1);
	}

	CloseHandle(hDriver);

}
