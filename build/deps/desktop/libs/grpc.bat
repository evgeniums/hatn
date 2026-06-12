SET lib_name=grpc

SET folder=%SRC%\%lib_name%
SET repo_path=https://github.com/grpc/%lib_name%

CALL %SCRIPTS_ROOT%/scripts/clonegit.bat

git checkout v%GRPC_VERSION%
if %errorlevel% neq 0 exit %errorlevel%
git submodule update --init --recursive
if %errorlevel% neq 0 exit %errorlevel%

cd %build_dir%

IF DEFINED CMAKE_MSVC_GENERATOR (

SET ARCH_CMAKE=-G "%CMAKE_MSVC_GENERATOR%" -A %MSVC_BUILD_ARCH% -T %MSVC_TOOLSET%

) ELSE (

SET ARCH_CMAKE=-G Ninja

)

cmake %ARCH_CMAKE% -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%DEPS_PREFIX% ^
        -DCMAKE_PREFIX_PATH=%DEPS_PREFIX% ^
        -DgRPC_INSTALL=ON ^
        -DgRPC_BUILD_TESTS=OFF ^
        -DgRPC_BUILD_CSHARP_EXT=OFF ^
        -DCMAKE_CXX_STANDARD=17 ^
        -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF ^
        -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF ^
        -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF ^
        -DgRPC_CARES_PROVIDER=package ^
        -DgRPC_SSL_PROVIDER=package ^
        "-Dc-ares_DIR=%DEPS_PREFIX%/lib/cmake/c-ares" ^
        "-DOPENSSL_INCLUDE_DIR=%DEPS_PREFIX%/include" ^
        "-DOPENSSL_CRYPTO_LIBRARY=%DEPS_PREFIX%/lib/libcrypto.lib" ^
        "-DOPENSSL_SSL_LIBRARY=%DEPS_PREFIX%/lib/libssl.lib" ^
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
