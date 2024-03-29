@ECHO OFF

IF "%MSYS_ROOT%"=="" SET MSYS_ROOT=C:\msys64

IF "%PYTHON_PATH%"=="" SET PYTHON_PATH=C:\Programs\Python
IF "%CMAKE_PATH%"=="" SET CMAKE_PATH=C:\Program Files\CMake\bin

IF "%BUILD_64%"=="1" SET PATH=%MSYS_ROOT%\mingw64\bin;%CMAKE_PATH%;%PYTHON_PATH%;%PATH%
IF NOT "%BUILD_64%"=="1" SET PATH=%MSYS_ROOT%\mingw32\bin;%CMAKE_PATH%;%PYTHON_PATH%;%PATH%

IF "%BUILD_64%"=="1" (
	SET MINGW_ARCH=x86_64
) ELSE (
	SET MINGW_ARCH=x86
)

IF "%DEPS_ROOT%"=="" (
	IF "%DEPS_UNIVERSAL_ROOT%"==""	(
		SET DEPS_ROOT=%WORKING_DIR%\..\deps\root-mingw-%MINGW_ARCH%
	) ELSE (
		SET DEPS_ROOT=%DEPS_UNIVERSAL_ROOT%\root-mingw-%MINGW_ARCH%
	)
)

IF "%BOOST_ROOT%"=="" SET BOOST_ROOT=%DEPS_ROOT%
IF "%OPENSSL_ROOT_DIR%"=="" SET OPENSSL_ROOT_DIR=%DEPS_ROOT%

IF "%BUILD_WORKERS%"=="" SET BUILD_WORKERS=6
