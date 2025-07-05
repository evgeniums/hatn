@ECHO OFF

ECHO "build.bat: CMAKE_CXX_STANDARD=%CMAKE_CXX_STANDARD%"
ECHO "build.bat: HATN_SMARTPOINTERS_STD=%HATN_SMARTPOINTERS_STD%"
ECHO "build.bat: HATN_TEST_NAME=%HATN_TEST_NAME%"

SET HATN_LIB=%1
SET HATN_COMPILER=%2
SET HATN_ARCH=%3
SET HATN_BUILD=%4
SET HATN_LINK=%5
SET HATN_PLUGINS=%6
SET mypath=%~dp0

ECHO "build.bat: Running build script %mypath%do-build.bat"
call %mypath%do-build.bat
if %errorlevel% neq 0 exit /b %errorlevel%
