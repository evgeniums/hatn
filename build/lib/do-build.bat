@ECHO OFF

SET build_start_dir=%CD%
SET WORKING_DIR=%CD%\build

IF EXIST  hatn.src (
    ECHO Do not run build scripts in source directory!
    ECHO Please, invoke %0 in some other working directory.
    EXIT /b 1
)

IF EXIST  ..\..\hatn.src (
    ECHO Do not run build scripts in source directory!
    ECHO Please, invoke %0 in some other working directory.
    EXIT /b 1
)

IF "%HATN_SRC%"=="" (
	SET HATN_SRC=%CD%\hatn
)
ECHO "HATN_SRC=%HATN_SRC%"

IF "%HATN_COMPILER%" == "gcc" SET HATN_COMPILER=mingw

IF NOT EXIST build mkdir build
cd build
IF NOT EXIST scripts mkdir scripts
IF NOT EXIST scripts\%HATN_LIB% (
	call %HATN_SRC%\build\lib\windows\generate-build-scripts.bat %HATN_LIB%
    if %errorlevel% neq 0 exit /b %errorlevel%
)
SET SCRIPT_NAME=%HATN_BUILD%-%HATN_LINK%-dev-%HATN_ARCH%
call scripts\%HATN_LIB%\%HATN_COMPILER%-%HATN_ARCH%\%SCRIPT_NAME%.bat
if %errorlevel% neq 0 exit /b %errorlevel%

cd %build_start_dir%
