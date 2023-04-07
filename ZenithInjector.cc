#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <filesystem>
#include <tchar.h>

#include "xorstr.hh"
#include "chalk.hh"
#include "Injector.hh"
#include "SimpleIni.hh"

int main() {
	Injector inj;
	CSimpleIniA ini;

    ini.LoadFile(xorstr_("config.ini"));
    const char* steam_module = ini.GetValue("files", xorstr_("steam_module"), nullptr);
    const char* cheat_dll = ini.GetValue("files", xorstr_("cheat_dll"), nullptr);
    const char* steam_method = ini.GetValue(xorstr_("injector"), xorstr_("steam_method"), nullptr);
    const char* cheat_method = ini.GetValue(xorstr_("injector"), xorstr_("cheat_method"), nullptr);

    if (steam_module == nullptr) { steam_module = xorstr_("VAK.dll"); }
    if (steam_method == nullptr) { steam_method = xorstr_("loadlib"); }
    if (cheat_method == nullptr) { cheat_method = xorstr_("manualmap"); }

    if (cheat_dll == nullptr) {
        if (IsDebuggerPresent() == false) {
            std::cout << xorstr_("Cheat cannot be null") << std::endl;
        }
        else {
            throw std::runtime_error(xorstr_("Cheat cannot be null"));
        }
    }

    try {
        inj.StartSteam(inj.GetSteamPath());
        inj.WaitForDLL(xorstr_("steam.exe"), xorstr_("serverbrowser.dll"));
        inj.Inject(inj.GetProcessID(xorstr_("steam.exe")), steam_module, steam_method);
        inj.StartCSGO();
        inj.WaitForDLL(xorstr_("csgo.exe"), xorstr_("serverbrowser.dll"));
        inj.Inject(inj.GetProcessID(xorstr_("csgo.exe")), cheat_dll, cheat_method);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << chalk::fg::BrightGreen.Wrap("Done!") << std::endl;
    return 0;
}