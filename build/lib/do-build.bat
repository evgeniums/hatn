@ECHO OFF

SET build_start_dir=%CD%

echo "PROJECT_WORKING_DIR=%PROJECT_WORKING_DIR%"

SET WORKING_DIR=%PROJECT_WORKING_DIR%
IF %WORKING_DIR%=="" SET WORKING_DIR=%CD%\build

ECHO "WORKING_DIR=%WORKING_DIR%"

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

IF "%HATN_PATH%"=="" (
	SET HATN_PATH=%CD%\hatn
)
ECHO "HATN_PATH=%HATN_PATH%"

IF "%HATN_COMPILER%" == "gcc" SET HATN_COMPILER=mingw

IF NOT EXIST %WORKING_DIR% (
echo "Creating working directory %WORKING_DIR%"
mkdir "%WORKING_DIR%"
echo "Created working directory %WORKING_DIR%"
)

cd %WORKING_DIR%

IF NOT EXIST scripts mkdir scripts
IF NOT EXIST scripts\%HATN_LIB% (
	call %HATN_PATH%\build\lib\windows\generate-build-scripts.bat %HATN_LIB% %HATN_COMPILER%
    if %errorlevel% neq 0 exit /b %errorlevel%
)
SET SCRIPT_NAME=%HATN_BUILD%-%HATN_LINK%-dev-%HATN_ARCH%
call scripts\%HATN_LIB%\%HATN_COMPILER%-%HATN_ARCH%\%SCRIPT_NAME%.bat
if %errorlevel% neq 0 exit /b %errorlevel%

cd %build_start_dir%
