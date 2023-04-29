# Zenith Injector (v1)
Really simple and messy injector / vac bypass for CS:GO

- Manual mapping: https://github.com/Zer0Mem0ry/ManualMap
- VAC bypass: https://github.com/zyhp/vac3_inhibitor/
---
# INI File
Make a INI file named `config.ini` and make sure to set its contents to this template:
```ini
[files]
steam_module=<steam module.dll / if there is none present will default to VAK.dll>
cheat_dll=<cheat.dll>

[injector]
steam_method=loadlib or manualmap
cheat_method=loadlib or manualmap
```

Example
---
```ini
[files]
steam_module=VAK.dll
cheat_dll=osiris.dll

[injector]
steam_method=loadlib
cheat_method=manualmap
```

# FAQ
1. Injection fails? Maybe your DLL is broken or the injection method is not supported by the cheat
2. Steam crashes while using VAK.dll? Run the injector as admin
3. Manualmap crashes and LoadLibrary doesn't work? Make sure to be compiling in Release x86

This doesn't help? Open an issue
