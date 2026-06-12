SET lib_name=sentry-native

SET folder=%SRC%\%lib_name%
SET repo_path=https://github.com/getsentry/sentry-native

REM Clone / pull the repository (clonegit.bat does not support tag args, so we
REM checkout the pinned version explicitly afterwards).
CALL %SCRIPTS_ROOT%/scripts/clonegit.bat

REM After clonegit.bat we are already cd'd into %folder%.
git checkout %SENTRY_VERSION%
if %errorlevel% neq 0 exit %errorlevel%
git submodule update --init --recursive
if %errorlevel% neq 0 exit %errorlevel%

cd %build_dir%

REM Use crashpad backend for maximum reliability on desktop.
IF DEFINED CMAKE_MSVC_GENERATOR (
SET ARCH_CMAKE=-G "%CMAKE_MSVC_GENERATOR%" -A %MSVC_BUILD_ARCH% -T %MSVC_TOOLSET%
) ELSE (

SET ARCH_CMAKE=-G Ninja

)


cmake %ARCH_CMAKE% -DCMAKE_INSTALL_PREFIX=%DEPS_PREFIX% ^
        -DCMAKE_BUILD_TYPE=Release ^
        -DSENTRY_BACKEND=crashpad ^
        -DSENTRY_BUILD_SHARED_LIBS=OFF ^
        -DSENTRY_BUILD_TESTS=OFF ^
        -DSENTRY_BUILD_EXAMPLES=OFF ^
        %folder%
if %errorlevel% neq 0 exit %errorlevel%

IF NOT DEFINED CMAKE_MSVC_GENERATOR (

cmake --build . --target install --config Release -- -j %BUILD_WORKERS%
if %errorlevel% neq 0 exit %errorlevel%

) ELSE (

cmake --build . --target install --config Release -- /m:1 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
if %errorlevel% neq 0 exit %errorlevel%
)

cd %WORKING_DIR%
