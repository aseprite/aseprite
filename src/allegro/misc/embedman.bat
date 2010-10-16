@ECHO OFF
REM *** embedman.bat is a helper tool for MSVC8.
REM     It embeds all XML manifest files into its corresponding executable.

REM ======== DISPATCHING ========
IF "%1" == "" GOTO usage
IF "%1" == "all" IF "%2" == "" GOTO search
IF "%2" == "" GOTO bad_usage
IF NOT EXIST "%1" GOTO file_not_found
IF NOT EXIST "%1.manifest" GOTO no_manifest

REM ======== EMBED MANIFEST ========
mt -nologo -manifest %1.manifest -outputresource:%1;%2
IF NOT "%ERRORLEVEL%" == "0" GOTO mt_fail
del %1.manifest
GOTO end

REM ======== SEARCH AND EMBED ========
:search
FOR /R %%v IN (*.exe) DO CALL %0 %%v 1 -silent
FOR /R %%v IN (*.scr) DO CALL %0 %%v 1 -silent
FOR /R %%v IN (*.dll) DO CALL %0 %%v 2 -silent

GOTO end

REM ======== ERRORS ========
:usage
ECHO.
ECHO Running embedman.bat with "all" as the first and only parameter causes it to 
ECHO search for all executables and DLLs with manifest files, and embed them.
ECHO.
ECHO %0 all
ECHO.
ECHO Alternatively, you can specify an exact file:
ECHO.
ECHO %0 file.exe 1
ECHO %0 file.dll 2
ECHO.
ECHO In every case, the manifest file must exist in the same folder with the same
ECHO name as the executable for it to work. The manifest file is then deleted.
GOTO end

:bad_usage
ECHO.
ECHO You must specify whether the file is an executable or DLL via the second
ECHO parameter.
ECHO.
ECHO 1 = Executable
ECHO 2 = DLL
GOTO end

:file_not_found
ECHO.
ECHO The file "%1" was not found.
GOTO end

:mt_fail
ECHO Unable to embed %1.manifest! Make sure mt.exe is in the path.
GOTO end

:no_manifest
IF NOT "%3" == "-silent" ECHO No manifest found for %1


REM ======== THE END ========
:end
