@echo off
set ctime=usr\bin\ctime.exe
set msvcdir="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\"
if not defined DevEnvDir call %msvcdir%vcvars64.bat >nul
rem set msvcdir="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\x86_amd64\"
rem if not defined DevEnvDir call %msvcdir%vcvarsx86_amd64.bat

if not exist bin mkdir bin
copy usr\lib\*.dll bin\ >nul 2>&1

rem Kill task if it's running
set name=LD42
taskkill /IM %name%.exe 1>NUL 2>&1

nmake /nologo -k -f windows.mak
set lasterror=%errorlevel%

if "%~1"=="run" goto run
goto Quit

:run
if %lasterror% LEQ 0 start bin\%name%.exe

:Quit

