@ECHO OFF

ECHO "do-build.bat: begin"

SET build_start_dir=%CD%

ECHO "do-build.bat: PROJECT_WORKING_DIR=%PROJECT_WORKING_DIR%"

SET WORKING_DIR=%PROJECT_WORKING_DIR%

IF "%PROJECT_WORKING_DIR%"=="" (
    ECHO "do-build.bat: PROJECT_WORKING_DIR is not set, using default %CD%\build"
    SET WORKING_DIR=%CD%\build
)

rem IF %PROJECT_WORKING_DIR%=="" SET WORKING_DIR=%CD%\build

ECHO "do-build.bat: WORKING_DIR=%WORKING_DIR%"

IF EXIST hatn.src (
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
    ECHO "do-build.bat: HATN_PATH is not set, using default %CD%\hatn"
    SET HATN_PATH=%CD%\hatn
)
ECHO "do-build.bat: HATN_PATH=%HATN_PATH%"

IF "%HATN_COMPILER%" == "gcc" SET HATN_COMPILER=mingw

IF NOT EXIST %WORKING_DIR% (
ECHO "do-build.bat: Creating working directory %WORKING_DIR%"
mkdir "%WORKING_DIR%"
ECHO "do-build.bat: Created working directory %WORKING_DIR%"
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

ECHO "do-build.bat: end"