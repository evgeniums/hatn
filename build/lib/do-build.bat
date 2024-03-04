@ECHO OFF

SET _self_dir=%CD%

IF "%HATN_COMPILER%" == "gcc" SET HATN_COMPILER=mingw

IF NOT EXIST build mkdir build
cd build
IF NOT EXIST scripts mkdir scripts
IF NOT EXIST scripts\%HATN_LIB% (
	call ..\hatn\build\lib\windows\generate-build-scripts.bat %HATN_LIB%
    if %errorlevel% neq 0 exit /b %errorlevel%
)
SET SCRIPT_NAME=%HATN_BUILD%-%HATN_LINK%-dev-%HATN_ARCH%
call scripts\%HATN_LIB%\%HATN_COMPILER%-%HATN_ARCH%\%SCRIPT_NAME%.bat
if %errorlevel% neq 0 exit /b %errorlevel%

cd %_self_dir%
