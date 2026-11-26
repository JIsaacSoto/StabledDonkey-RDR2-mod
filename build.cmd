@echo off
rem Builds build\Release\StabledDonkey.asi with the CMake bundled in Visual
rem Studio (found with vswhere). Set CMAKE to use another cmake.exe.
setlocal
cd /d "%~dp0"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if defined CMAKE goto configure
if not exist "%VSWHERE%" goto fallback
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`) do set "CMAKE=%%i"
if defined CMAKE goto configure
:fallback
set "CMAKE=cmake"

:configure
if exist build\CMakeCache.txt goto build
"%CMAKE%" -S . -B build -A x64 || exit /b 1

:build
"%CMAKE%" --build build --config Release || exit /b 1
echo OK: %~dp0build\Release\StabledDonkey.asi
