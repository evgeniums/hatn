SET lib_name=gflags

SET folder=%SRC%\%lib_name%
SET repo_path=https://github.com/gflags/%lib_name%

CALL %SCRIPTS_ROOT%/scripts/clonegit.bat

cd %build_dir%

IF DEFINED CMAKE_MSVC_GENERATOR (

SET ARCH_CMAKE=-G "%CMAKE_MSVC_GENERATOR%" -A %MSVC_BUILD_ARCH% -T %MSVC_TOOLSET%

)

cmake %ARCH_CMAKE% -DCMAKE_INSTALL_PREFIX=%DEPS_PREFIX% -DCMAKE_BUILD_TYPE=Release -DREGISTER_INSTALL_PREFIX=0 %folder%
if %errorlevel% neq 0 exit %errorlevel%
@ECHO OFF

IF NOT DEFINED CMAKE_MSVC_GENERATOR (

cmake --build . --target install -- -j %BUILD_WORKERS%
if %errorlevel% neq 0 exit %errorlevel%

) ELSE (

cmake --build . --target install --config Release -- /m:1 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
if %errorlevel% neq 0 exit %errorlevel%

rem cmake --build . --target install --config Debug -- /m:1 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
rem if %errorlevel% neq 0 exit %errorlevel%
)

cd %WORKING_DIR%