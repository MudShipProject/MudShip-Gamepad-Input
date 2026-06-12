@echo off
setlocal
REM Build MS_GamepadBridge.dll and deploy into the package Runtime/Plugins/x86_64.
REM Requires: VS2022 (C++ workload) + Windows SDK 10.0.26100+ (GameInput.h / GameInput.lib)
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" 10.0.26100.0
cd /d "%~dp0"
cl /nologo /LD /EHsc /O2 /std:c++17 /utf-8 /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" GameInputBridge.cpp /Fe:MS_GamepadBridge.dll /link /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64" GameInput.lib
if errorlevel 1 goto :fail

copy /Y MS_GamepadBridge.dll "..\..\Runtime\Plugins\x86_64\MS_GamepadBridge.dll" >nul
if errorlevel 1 (
  echo *** Deploy to Runtime\Plugins failed. If Unity locks the DLL, close Unity and retry. ***
  exit /b 1
)
echo ==== OK: built and deployed to Runtime\Plugins\x86_64 ====
exit /b 0

:fail
echo ==== BUILD FAILED ====
exit /b 1
