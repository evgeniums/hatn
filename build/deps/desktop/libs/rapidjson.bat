SET lib_name=rapidjson

SET folder=%SRC%\%lib_name%
SET repo_path=https://github.com/Tencent/%lib_name%

CALL %SCRIPTS_ROOT%scripts/clonegit.bat

cd %folder%
SET patch_file=%SCRIPTS_ROOT%libs/rapidjson.patch
%PATCH% -R -p0 -s -f --dry-run <%patch_file% > NUL
if %errorlevel% neq 0 (
  echo "rapidjson distro must be patched with %patch_file%"
  %PATCH% -p0 <%patch_file%
  echo "rapidjson patched"
)

cd %build_dir%

IF %ADDRESS_MODEL%==32 SET ARCH_CMAKE=-A %MSVC_BUILD_ARCH% -T %MSVC_TOOLSET%

cmake %ARCH_CMAKE% ^
    -DCMAKE_INSTALL_PREFIX=%DEPS_PREFIX% ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DRAPIDJSON_BUILD_EXAMPLES=Off ^
    -DRAPIDJSON_BUILD_TESTS=Off ^
    -DRAPIDJSON_BUILD_DOC=Off ^
    %folder%

if %errorlevel% neq 0 exit %errorlevel%
@ECHO OFF
cmake --build . --target install --config Release -- /m:1 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
if %errorlevel% neq 0 exit %errorlevel%

cd %WORKING_DIR%