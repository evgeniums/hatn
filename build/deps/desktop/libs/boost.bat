SET boost_underscore_version=%BOOST_VERSION%
SETLOCAL ENABLEDELAYEDEXPANSION
SET boost_underscore_version=%boost_underscore_version:.=_%

SET folder=%SRC%\boost_%boost_underscore_version%
SET archive=boost_%boost_underscore_version%.7z

SET download_link=https://archives.boost.io/release/%BOOST_VERSION%/source/%archive%

CALL %SCRIPTS_ROOT%/scripts/downloadandunpack.bat  

SET patch_file=%SCRIPTS_ROOT%libs/boost_win.patch
%PATCH% -R -p0 -s -f --dry-run <%patch_file% > NUL
if %errorlevel% neq 0 (
  echo "boost must be patched with %patch_file%"
  %PATCH% -u -b boost\beast\core\file_win32.hpp -i %patch_file%
  echo "boost patched"
)

call "bootstrap" %COMPILER%
IF %errorlevel% NEQ 0 (
    ECHO Failed to build boost
    cd %WORKING_DIR%
    EXIT /b %errorlevel%
)    

IF "%HATN_ARCH%"=="x86" (
	SET ADDR_MODEL=32
) ELSE (
	SET ADDR_MODEL=64
)

IF "%HATN_ARCH%"=="x86_64" SET BOOST_ARCH=x86
IF "%HATN_ARCH%"=="arm64" SET BOOST_ARCH=arm

.\b2 ^
   --with-date_time ^
    --with-exception ^
    --with-filesystem ^
    --with-iostreams ^
    --with-locale ^
    --with-program_options ^
    --with-random ^
    --with-regex ^
    --with-system ^
    --with-test ^
    --with-thread ^
    --with-timer ^
    --with-container ^
    toolset=%COMPILER%%TOOLCHAIN_LINE%%COMPILER_VERSION% ^
    address-model=%ADDR_MODEL% ^
    architecture=%BOOST_ARCH% ^
    link=shared ^
    install ^
    --prefix=%DEPS_PREFIX% ^
    --build-dir=%build_dir% ^
    -j%BUILD_WORKERS%

IF %errorlevel% NEQ 0 (
    ECHO Failed to build boost
)    

cd %WORKING_DIR%
