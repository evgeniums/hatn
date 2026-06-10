SET lib_name=utf8proc

SET folder=%SRC%\%lib_name%
SET repo_path=https://github.com/JuliaStrings/%lib_name%

CALL %SCRIPTS_ROOT%/scripts/clonegit.bat

cd %build_dir%

IF DEFINED CMAKE_MSVC_GENERATOR (

SET ARCH_CMAKE=-G "%CMAKE_MSVC_GENERATOR%" -A %MSVC_BUILD_ARCH% -T %MSVC_TOOLSET%

) ELSE {

SET ARCH_CMAKE=-G Ninja

}

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
