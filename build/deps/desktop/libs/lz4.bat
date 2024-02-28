SET lib_name=lz4

SET folder=%SRC%\%lib_name%
SET repo_path=https://github.com/lz4/%lib_name%

CALL %SCRIPTS_ROOT%/scripts/clonegit.bat

msbuild.exe %folder%\build\VS2022\lz4.sln /t:liblz4 /p:Configuration=Release /p:PlatformToolset=%MSVC_COMPILER% /p:Platform="%MSVC_BUILD_ARCH%"
if %errorlevel% neq 0 exit %errorlevel%
for /R %folder% %%f in (*.h) do copy %%f %DEPS_PREFIX%\include\ 
if %errorlevel% neq 0 exit %errorlevel%
for /R %folder% %%f in (*.lib) do copy %%f %DEPS_PREFIX%\lib\ 
if %errorlevel% neq 0 exit %errorlevel%

cd %WORKING_DIR%