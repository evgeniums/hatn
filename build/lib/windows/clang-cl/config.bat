@ECHO OFF

IF "%GETTEXT_PATH%"=="" SET GETTEXT_PATH=C:\data\libs-dev\gettext\bin
IF "%PYTHON_PATH%"=="" SET PYTHON_PATH=C:\Programs\Python
IF "%CMAKE_PATH%"=="" SET CMAKE_PATH=C:\Program Files\CMake\bin
SET PATH=%PATH%;%CMAKE_PATH%;%PYTHON_PATH%

IF "%MSVC_ROOT%"=="" SET MSVC_ROOT=C:\Program Files\Microsoft Visual Studio\18\Community

REM MSVC environment is still required for the Windows SDK, MSVC STL and link.exe;
REM MSVC_COMPILER_VERSION also names the MSVC-built dependencies root (hybrid mode).
IF NOT DEFINED MSVC_COMPILER_VERSION (
SET MSVC_COMPILER_VERSION=14.51
)
ECHO "Using clang-cl compiler in MSVC %MSVC_COMPILER_VERSION% environment"

SET MSVCARGS=%MSVC_ROOT%\VC\Auxiliary\Build\vcvarsall.bat

IF "%ADDRESS_SPACE%"=="arm64" (
	ECHO -- Building arm64 platform
	SET MSVC_ARCH=arm64
	SET NAME_SUFFIX="_arm64"
	SET MSVC_FULL_ARCH=arm64
	SET CLANG_TARGET=arm64-pc-windows-msvc
	SET LLVM_HOST_DIR=ARM64
) ELSE (
	IF "%ADDRESS_SPACE%"=="x86_64" (
		ECHO -- Building x64 platform
		SET MSVC_ARCH=x64
		SET NAME_SUFFIX="_x64"
		SET MSVC_FULL_ARCH=x86_64
		SET CLANG_TARGET=x86_64-pc-windows-msvc
		SET LLVM_HOST_DIR=x64
	) ELSE (
		ECHO -- Building x86 platform
		SET MSVC_ARCH=x86
		SET NAME_SUFFIX=
		SET MSVC_FULL_ARCH=x86
		SET CLANG_TARGET=i686-pc-windows-msvc
		SET LLVM_HOST_DIR=x64
	)
)

REM Default to the LLVM bundled with Visual Studio ("C++ Clang tools for Windows" component).
REM Override LLVM_ROOT for a standalone install, e.g. SET LLVM_ROOT=C:\Program Files\LLVM
IF NOT DEFINED LLVM_ROOT SET LLVM_ROOT=%MSVC_ROOT%\VC\Tools\Llvm\%LLVM_HOST_DIR%
SET PATH=%LLVM_ROOT%\bin;%PATH%
ECHO "Using LLVM_ROOT=%LLVM_ROOT%"

REM Hybrid mode: dependencies are built with MSVC, keep msvc-named deps root.
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
