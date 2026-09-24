SET lib_name=ogg

SET folder=%SRC%\%lib_name%
SET repo_path=https://github.com/xiph/%lib_name%

REM clonegit.bat does not support tag args, so pin the version explicitly afterwards -- same
REM workaround sentry.bat and hunspell.bat use.
CALL %SCRIPTS_ROOT%/scripts/clonegit.bat

REM After clonegit.bat we are already cd'd into %folder%.
git checkout v%OGG_VERSION%
if %errorlevel% neq 0 exit %errorlevel%

cd %build_dir%

IF DEFINED CMAKE_MSVC_GENERATOR (

SET ARCH_CMAKE=-G "%CMAKE_MSVC_GENERATOR%" -A %MSVC_BUILD_ARCH% -T %MSVC_TOOLSET%

) ELSE (

SET ARCH_CMAKE=-G Ninja

)

cmake %ARCH_CMAKE% -DCMAKE_INSTALL_PREFIX=%DEPS_PREFIX% -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DINSTALL_DOCS=OFF %folder%
if %errorlevel% neq 0 exit %errorlevel%
@ECHO OFF

IF NOT DEFINED CMAKE_MSVC_GENERATOR (

cmake --build . --target install --config Release -- -j %BUILD_WORKERS%
if %errorlevel% neq 0 exit %errorlevel%

) ELSE (

cmake --build . --target install --config Release -- /m:1 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
if %errorlevel% neq 0 exit %errorlevel%

)

cd %WORKING_DIR%
