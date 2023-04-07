#include "Injector.hh"

#include <iostream>
#include <Psapi.h>
#include <locale>
#include <codecvt>

#include "xorstr.hh"
#include "chalk.hh"
#include <filesystem>

std::string Injector::GetSteamPath() {
	HKEY hKey;
	LONG lResult;
	DWORD dwType = REG_SZ;
	DWORD dwSize = MAX_PATH;
	char szSteamPath[MAX_PATH];

	// Open the registry key
	lResult = RegOpenKeyEx(HKEY_CURRENT_USER, xorstr_("SOFTWARE\\Valve\\Steam"), 0, KEY_READ, &hKey);
	if (lResult != ERROR_SUCCESS)
	{
		throw std::runtime_error(xorstr_("Couldn't open Steam registery hive"));
	}

	// Read the Steam path value from the registry
	lResult = RegQueryValueEx(hKey, xorstr_("SteamExe"), NULL, &dwType, (LPBYTE)szSteamPath, &dwSize);
	if (lResult != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		throw std::runtime_error(xorstr_("Couldn't get steam.exe path"));
	}

	// Close the registry key
	RegCloseKey(hKey);

	// Convert the path to a std::string and return it
	std::string strSteamPath(szSteamPath, dwSize);
	return strSteamPath;
}

DWORD Injector::GetProcessID(const char* processName) {
	DWORD processId = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 processEntry = { 0 };
		processEntry.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(snapshot, &processEntry))
		{
			do
			{
				if (_stricmp(processEntry.szExeFile, processName) == 0)
				{
					processId = processEntry.th32ProcessID;
					break;
				}
			} while (Process32Next(snapshot, &processEntry));
		}
		CloseHandle(snapshot);
	}
	return processId;
}

bool Injector::KillProcess(const DWORD& pid)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	if (hProcess == NULL) {
		std::cerr << xorstr_("Error: Unable to open process ") << pid << "." << std::endl;
		return false;
	}

	if (TerminateProcess(hProcess, 0)) {
		std::cout << xorstr_("Process ") << pid << " killed." << std::endl;
		CloseHandle(hProcess);
		return true;
	}
	else {
		std::cerr << xorstr_("Error: Unable to kill process ") << pid << "." << std::endl;
		CloseHandle(hProcess);
		return false;
	}
}

bool Injector::IsProcessOpen(const std::string& processName)
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		return false;
	}

	cProcesses = cbNeeded / sizeof(DWORD);
	for (unsigned int i = 0; i < cProcesses; ++i)
	{
		if (aProcesses[i] != 0)
		{
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
			if (hProcess)
			{
				TCHAR szProcessName[MAX_PATH] = TEXT("???");
				HMODULE hModule;
				DWORD cbNeeded;

				if (EnumProcessModules(hProcess, &hModule, sizeof(hModule), &cbNeeded))
				{
					GetModuleBaseName(hProcess, hModule, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
				}

				if (strcmp(szProcessName, processName.c_str()) == 0)
				{
					CloseHandle(hProcess);
					return true;
				}

				CloseHandle(hProcess);
			}
		}
	}

	return false;
}

bool Injector::IsDllLoaded(DWORD processId, const char* dllName)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	bool result = false;
	MODULEENTRY32 moduleEntry;
	moduleEntry.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(snapshot, &moduleEntry))
	{
		do
		{
			if (_strcmpi(moduleEntry.szModule, dllName) == 0)
			{
				result = true;
				break;
			}
		} while (Module32Next(snapshot, &moduleEntry));
	}

	CloseHandle(snapshot);
	return result;
}

bool Injector::StartSteam(const std::string& steamExePath) {
	// CreateProcess requires a non-const char* for the command line argument
	// Convert the string to a non-const char* using c_str()
	const char* commandLine = steamExePath.c_str();

	// Set up the process startup information
	STARTUPINFO startupInfo;
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);

	// Set up the process information
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&processInfo, sizeof(processInfo));

	// Create the process
	if (CreateProcess(nullptr, const_cast<char*>(commandLine), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo))
	{
		// Close the process and thread handles (we don't need them)
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		// The process was successfully created
		return true;
	}
	else
	{
		// An error occurred while creating the process
		return false;
	}
}

void Injector::StartCSGO()
{
	HINSTANCE result = ShellExecute(NULL, "open", xorstr_("steam://rungameid/730"), NULL, NULL, SW_SHOWNORMAL);

	if ((int)result <= 32) {
		IsDebuggerPresent() == false ?
			std::cout << chalk::fg::Red.Wrap(xorstr_("Failed to open CS:GO")) << std::endl
			: throw std::runtime_error(xorstr_("Failed to open CS:GO"));
	}
}

void Injector::Inject(DWORD pid, const char* dllPath, const char* method)
{
	if (strcmp(method,"loadlib") == 0) {
		this->LoadLib(pid, std::filesystem::absolute(dllPath).string().c_str());
	}
	else if (strcmp(method, "manualmap") == 0) {
		this->ManualMap(pid, std::filesystem::absolute(dllPath).string().c_str());
	}
	else {
		throw std::runtime_error(xorstr_("Steam injection method not valid"));
	}	
}

void Injector::LoadLib(DWORD processId, const char* dllPath)
{
	HANDLE steamProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	LPVOID lpDllPath = VirtualAllocEx(steamProcess, NULL, strlen(dllPath), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	WriteProcessMemory(steamProcess, lpDllPath, dllPath, strlen(dllPath), NULL);

	LPVOID lpLoadLibraryA = GetProcAddress(GetModuleHandleA(xorstr_("kernel32.dll")), xorstr_("LoadLibraryA"));
	HANDLE steamThread = CreateRemoteThread(steamProcess, NULL, 0, (LPTHREAD_START_ROUTINE)lpLoadLibraryA, lpDllPath, 0, NULL);

	WaitForSingleObject(steamThread, INFINITE);
	VirtualFreeEx(steamProcess, lpDllPath, 0, MEM_RELEASE);

	CloseHandle(steamThread);
	CloseHandle(steamProcess);
}
