#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include <vector>

#include "manualmap.hh"

class Injector {
	public:
		Injector() {
			std::atexit([]() { std::cin.get(); });

			HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			DWORD mode;

			GetConsoleMode(hConsole, &mode);
			mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(hConsole, mode);
		}

		std::string GetSteamPath();
		DWORD GetProcessID(const char* processName);
		bool KillProcess(const DWORD& pid);
		bool IsProcessOpen(const std::string& processName);
		void WaitForDLL(const char* processName, const char* dllName) { while (!this->IsDllLoaded(this->GetProcessID(processName), dllName)) { Sleep(10); } };

		bool StartSteam(const std::string& steamExePath);
		void StartCSGO();

		void Inject(DWORD pid, const char* dllPath, const char* method);
	private:
		bool IsDllLoaded(DWORD processId, const char* dllName);
		void LoadLib(DWORD processId, const char* dllPath);
		void ManualMap(DWORD processId, const char* dllPath) { MM::map(processId, dllPath); };
};
