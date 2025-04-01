SET lib_name=rocksdb

SET folder=%SRC%\%lib_name%
SET repo_path=https://github.com/facebook/%lib_name%

CALL %SCRIPTS_ROOT%/scripts/clonegit.bat

cd %build_dir%

set GFLAGS_INCLUDE=%DEPS_PREFIX%/include
set GFLAGS_LIB_RELEASE=%DEPS_PREFIX%/lib/gflags_static.lib
set GFLAGS_LIB_DEBUG=%DEPS_PREFIX%/lib/gflags_static.lib
rem set GFLAGS_LIB_DEBUG=%DEPS_PREFIX%/lib/gflags_static_debug.lib

set LZ4_INCLUDE=%DEPS_PREFIX%/include
set LZ4_LIB_RELEASE=%DEPS_PREFIX%/lib/lz4_static.lib
set LZ4_LIB_DEBUG=%DEPS_PREFIX%/lib/lz4_static.lib

IF DEFINED CMAKE_MSVC_GENERATOR (

SET ARCH_CMAKE=-G "%CMAKE_MSVC_GENERATOR%" -A %MSVC_BUILD_ARCH% -T %MSVC_TOOLSET%

)

cmake %ARCH_CMAKE% -DCMAKE_INSTALL_PREFIX=%DEPS_PREFIX% ^
		-DCMAKE_BUILD_TYPE=Release ^
        -DWITH_TESTS=0 ^
		-DWITH_BENCHMARK_TOOLS=0 ^
        -DFAIL_ON_WARNINGS=0 ^
        -DWITH_GFLAGS=1 ^
        -DWITH_LZ4=1 ^
        -DROCKSDB_INSTALL_ON_WINDOWS=1 ^
        -DCMAKE_FIND_USE_PACKAGE_REGISTRY=0 ^
         %folder%
if %errorlevel% neq 0 exit %errorlevel%

IF NOT DEFINED CMAKE_MSVC_GENERATOR (

cmake --build . --target install --config Release -- -j %BUILD_WORKERS%
if %errorlevel% neq 0 exit %errorlevel%        

) ELSE (

cmake --build . --target install --config Release -- /m:1 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
rem cmake --build . --target install --config Debug -- /m:1 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
if %errorlevel% neq 0 exit %errorlevel%
)

cd %WORKING_DIR%