@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if errorlevel 1 (
  echo Visual Studio x86 Native Tools Command Prompt penceresinde calistirin.
  exit /b 1
)
rc /nologo /fo app.res app.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /W4 /MT miran_control.cpp app.res /Fe:MiranControl32.exe /link /SUBSYSTEM:WINDOWS /MACHINE:X86 comctl32.lib shell32.lib ole32.lib uuid.lib user32.lib gdi32.lib uxtheme.lib
if errorlevel 1 exit /b 1
echo MiranControl32.exe olusturuldu.
