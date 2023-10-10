#include "injector.h"


#include <stdio.h>
#include <string>
#include <iostream>

using namespace std;

int wmain(LPVOID lpParam) {

	const wchar_t* dllPath = L"C:\\AMD\\AMD-Software-PRO-Edition-22.Q4-Win10-Win11-Nov15\\Bin64\\ADD_COMMON.dll";
	
	wifstream pidFile(L"C:\\AMD\\AMD-Software-PRO-Edition-22.Q4-Win10-Win11-Nov15\\Bin64\\logs.log");
	DWORD PID = 0;

	if (pidFile.is_open()) {
		pidFile >> PID;
		pidFile.close();
	}
	else {
		wcerr << L"Error opening pid.txt file." << endl;
		return -1;
	}

	TOKEN_PRIVILEGES priv = { 0 };
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		priv.PrivilegeCount = 1;
		priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid))
			AdjustTokenPrivileges(hToken, FALSE, &priv, 0, NULL, NULL);

		CloseHandle(hToken);
	}

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
	if (!hProc) {
		DWORD Err = GetLastError();
		printf("OpenProcess failed: 0x%X\n", Err);
		system("PAUSE");
		return -2;
	}

	if (GetFileAttributes(dllPath) == INVALID_FILE_ATTRIBUTES) {
		printf("Dll file doesn't exist\n");
		return -4;
	}

	std::ifstream File(dllPath, std::ios::binary | std::ios::ate);

	if (File.fail()) {
		printf("Opening the file failed: %X\n", (DWORD)File.rdstate());
		File.close();
		return -5;
	}

	auto FileSize = File.tellg();
	if (FileSize < 0x1000) {
		printf("Filesize invalid.\n");
		File.close();
		return -6;
	}

	BYTE * pSrcData = new BYTE[(UINT_PTR)FileSize];
	if (!pSrcData) {
		printf("Can't allocate dll file.\n");
		File.close();
		return -7;
	}

	File.seekg(0, std::ios::beg);
	File.read((char*)(pSrcData), FileSize);
	File.close();

	printf("Mapping...\n");
	if (!ManualMapDll(hProc, pSrcData, FileSize)) {
		delete[] pSrcData;
		CloseHandle(hProc);
		printf("Error while mapping.\n");
		system("PAUSE");
		return -8;
	}
	delete[] pSrcData;

	CloseHandle(hProc);
	printf("OK\n");
	return 0;
}

DWORD WINAPI ThreadFunction(LPVOID lpParam) {
	return wmain(lpParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		HANDLE hThread = CreateThread(NULL, 0, ThreadFunction, NULL, 0, NULL);
		if (hThread == NULL) {
			return FALSE;
		}
		CloseHandle(hThread);
	}
	return TRUE;
}