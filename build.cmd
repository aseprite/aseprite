@echo off

rem If cl.exe is not available, we try to run the vcvars64.bat
where cl.exe >nul 2>nul
if %errorlevel%==1 (
   set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
   if exist "%VSWHERE%" (
      for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
         set "VS_INSTALL_DIR=%%i"
      )
   )
   
   if defined VS_INSTALL_DIR (
      if exist "%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat" (
         @call "%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat"
      )
   ) else (
      rem Fallback options if vswhere fails or is not present
      if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
         @call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
      ) else (
         if exist "%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
            @call "%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
         )
      )
   )
)

rem Add an extra path element which will be invalidated by Git Bash.
rem In this way we avoid invalidating the PATH location where cl.exe is.
set PATH=.;%PATH%

powershell -ExecutionPolicy Bypass -File .\build.ps1 %*
pause
