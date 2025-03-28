SET lib_name=gflags

SET folder=%SRC%\%lib_name%
SET repo_path=https://github.com/gflags/%lib_name%

CALL %SCRIPTS_ROOT%/scripts/clonegit.bat

cd %build_dir%

IF %ADDRESS_MODEL%==32 SET ARCH_CMAKE=-A %MSVC_BUILD_ARCH% -T %MSVC_TOOLSET%

cmake %ARCH_CMAKE% -DCMAKE_INSTALL_PREFIX=%DEPS_PREFIX% -DCMAKE_BUILD_TYPE=Release -DREGISTER_INSTALL_PREFIX=0 %folder%
if %errorlevel% neq 0 exit %errorlevel%
@ECHO OFF

cmake --build . --target install --config Release -- /m:1 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
if %errorlevel% neq 0 exit %errorlevel%

cmake --build . --target install --config Debug -- /m:1 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
if %errorlevel% neq 0 exit %errorlevel%

cd %WORKING_DIR%