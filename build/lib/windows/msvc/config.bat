@ECHO OFF

IF "%GETTEXT_PATH%"=="" SET GETTEXT_PATH=C:\data\libs-dev\gettext\bin
IF "%PYTHON_PATH%"=="" SET PYTHON_PATH=C:\Programs\Python
IF "%CMAKE_PATH%"=="" SET CMAKE_PATH=C:\Program Files\CMake\bin
SET PATH=%PATH%;%CMAKE_PATH%;%PYTHON_PATH%

IF "%MSVC_ROOT%"=="" SET MSVC_ROOT=C:\Program Files\Microsoft Visual Studio\2022\Community

IF NOT DEFINED MSVC_COMPILER_VERSION (
SET MSVC_COMPILER_VERSION=14.3
)
FOR /f "tokens=1,2 delims=." %%a IN ("%MSVC_COMPILER_VERSION%") do SET MSVC_COMPILER=v%%a%%b
ECHO "Using %MSVC_COMPILER_VERSION% (%MSVC_COMPILER%) toolset"

SET MSVCARGS=%MSVC_ROOT%\VC\Auxiliary\Build\vcvarsall.bat

IF "%ADDRESS_SPACE%"=="arm64" (
	ECHO -- Building arm64 platform
	SET MSVC_ARCH=arm64
	SET CMAKE_PLATFORM=arm64
	SET NAME_SUFFIX="_arm64"
	SET MSVC_TOOLSET="%MSVC_COMPILER%,host=arm64"
	SET MSVC_FULL_ARCH=arm64
	SET MSVC_BUILD_ARCH=arm64	
) ELSE (
	IF "%ADDRESS_SPACE%"=="x86_64" (
		ECHO -- Building x64 platform
		SET MSVC_ARCH=x64
		SET CMAKE_PLATFORM=x64
		SET NAME_SUFFIX="_x64"
		SET MSVC_TOOLSET="%MSVC_COMPILER%,host=x64"
		SET MSVC_FULL_ARCH=x86_64
		SET MSVC_BUILD_ARCH=x64	
	) ELSE (
		ECHO -- Building x86 platform
		SET MSVC_ARCH=x86
		SET CMAKE_PLATFORM=
		SET NAME_SUFFIX=
		SET MSVC_TOOLSET=%MSVC_COMPILER%
		SET MSVC_FULL_ARCH=x86
		SET MSVC_BUILD_ARCH=Win32	
	)
)

IF "%DEPS_ROOT%"=="" (
	IF "%DEPS_UNIVERSAL_ROOT%"==""	(
		SET DEPS_ROOT=%WORKING_DIR%\..\deps\root-msvc-%MSVC_COMPILER_VERSION%-%MSVC_FULL_ARCH%
	) ELSE (
		SET DEPS_ROOT=%DEPS_UNIVERSAL_ROOT%\root-msvc-%MSVC_COMPILER_VERSION%-%MSVC_FULL_ARCH%
	)
)

ECHO "WORKING_DIR="%WORKING_DIR%"
ECHO "DEPS_ROOT="%DEPS_ROOT%"

IF "%BOOST_ROOT%"=="" SET BOOST_ROOT=%DEPS_ROOT%
IF "%OPENSSL_ROOT_DIR%"=="" SET OPENSSL_ROOT_DIR=%DEPS_ROOT%

IF "%BUILD_WORKERS%"=="" SET BUILD_WORKERS=6