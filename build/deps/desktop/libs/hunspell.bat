SET lib_name=hunspell

SET folder=%SRC%\%lib_name%
SET repo_path=https://github.com/hunspell/hunspell

REM clonegit.bat does not support tag args, so pin the version explicitly afterwards -- same
REM workaround sentry.bat uses for SENTRY_VERSION.
CALL %SCRIPTS_ROOT%/scripts/clonegit.bat

REM After clonegit.bat we are already cd'd into %folder%.
git checkout v%HUNSPELL_VERSION%
if %errorlevel% neq 0 exit %errorlevel%

REM hunspell ships autotools plus checked-in MSVC projects and no CMake build of its own -- copy
REM the shim over the clone's own (nonexistent) CMakeLists.txt, same source file the .sh variant
REM uses, so every platform builds it identically.
COPY /Y %SCRIPTS_ROOT%\libs\hunspell-CMakeLists.txt %folder%\CMakeLists.txt
if %errorlevel% neq 0 exit %errorlevel%

cd %build_dir%

IF DEFINED CMAKE_MSVC_GENERATOR (

SET ARCH_CMAKE=-G "%CMAKE_MSVC_GENERATOR%" -A %MSVC_BUILD_ARCH% -T %MSVC_TOOLSET%

) ELSE (

SET ARCH_CMAKE=-G Ninja

)

cmake %ARCH_CMAKE% -DCMAKE_INSTALL_PREFIX=%DEPS_PREFIX% -DCMAKE_BUILD_TYPE=Release %folder%
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
