#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <stdio.h>
#include <string>
#include "loader.h"
#include "tools.h"
#include "funcs.h"



int main()
{
	printf("[+] DriverLoader and some features by bieljtvz\n");
	system("pause");
	system("cls");

    if (!open_scm())
    {
        printf("[-] Failed to open handle to SCM!\n");
        return EXIT_FAILURE;
    }

    if (!load_driver())
    {

        printf("[-] Failed to loader a driver\n");

        // Do cleanup for our driver
        //
        unload_driver();     

        // Close handle to SCM
        //
        close_scm();

        Sleep(5000);

        return EXIT_FAILURE;
    }

    printf("[+] Driver probably loaded\n");
   // system("pause");

    //
    //Call some features and when they terminate will go next
   init_control();

    while (!GetAsyncKeyState(VK_END) & 1)
        Sleep(100);

  	//
	//Unload Driver
    if (!unload_driver())
        printf("[-] Failed to unload a driver\n");
    
    // Close handle to SCM
    //
    close_scm();

    printf("[+] Driver probably unloaded ");
    Sleep(5000);
    exit(0);

	getchar();
}