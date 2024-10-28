@echo off

rem If cl.exe is not available, we try to run the vcvars64.bat
where cl.exe >nul 2>nul
if %errorlevel%==1 (
   @call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

rem Add an extra path element which will be invalidated by Git Bash.
rem In this way we avoid invalidating the PATH location where cl.exe is.
set PATH=.;%PATH%

powershell -ExecutionPolicy Bypass -File .\build.ps1 %*
pause
