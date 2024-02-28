SET boost_underscore_version=%BOOST_VERSION%
SETLOCAL ENABLEDELAYEDEXPANSION
SET boost_underscore_version=%boost_underscore_version:.=_%

SET folder=%SRC%\boost_%boost_underscore_version%
SET archive=boost_%boost_underscore_version%.7z

SET download_link=https://boostorg.jfrog.io/artifactory/main/release/%BOOST_VERSION%/source/%archive%

CALL %SCRIPTS_ROOT%/scripts/downloadandunpack.bat  

call "bootstrap" %COMPILER%
IF %errorlevel% NEQ 0 (
    ECHO Failed to build boost
    cd %WORKING_DIR%
    EXIT /b %errorlevel%
)    

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
    address-model=%ADDRESS_MODEL% ^
    architecture=x86 ^
    link=shared ^
    install ^
    --prefix=%DEPS_PREFIX% ^
    --build-dir=%build_dir% ^
    -j%BUILD_WORKERS%

IF %errorlevel% NEQ 0 (
    ECHO Failed to build boost
)    

cd %WORKING_DIR%