@echo off
REM Show DLL exports. Expect: GIB_Init / GIB_Poll / GIB_Shutdown
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "%~dp0"
if exist MS_GamepadBridge.dll (
  dumpbin /nologo /exports MS_GamepadBridge.dll
) else (
  dumpbin /nologo /exports "..\..\Runtime\Plugins\x86_64\MS_GamepadBridge.dll"
)
