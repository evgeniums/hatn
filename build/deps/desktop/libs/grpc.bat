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

REM Optional: compiler-selection cmake args injected by the caller.
REM Set COMPILER_CMAKE to e.g. "-DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
REM -DCMAKE_C_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_CXX_COMPILER_TARGET=..."
REM when building with clang-cl.  Empty (default) means use the ambient MSVC compiler.
IF NOT DEFINED COMPILER_CMAKE SET COMPILER_CMAKE=

REM Optional: linker cmake arg.  Set to e.g. "-DCMAKE_LINKER=C:\path\link.exe" (outer
REM quotes required when the path contains spaces).  Empty by default.
IF NOT DEFINED LINKER_CMAKE SET LINKER_CMAKE=

REM Optional: prefix where system deps (c-ares, OpenSSL) live.
REM When building gRPC with clang-cl but c-ares/OpenSSL were built with MSVC, set this to
REM the MSVC deps root so that the lookup paths stay correct.  Defaults to DEPS_PREFIX.
IF NOT DEFINED SYSTEM_DEPS_PREFIX SET "SYSTEM_DEPS_PREFIX=%DEPS_PREFIX%"

REM Build type — override with GRPC_BUILD_TYPE=RelWithDebInfo to get debug symbols while
REM keeping /MD (matching a Release client).  Default is Release.
IF NOT DEFINED GRPC_BUILD_TYPE SET GRPC_BUILD_TYPE=Release

REM Debug-info format override.  With clang-cl, /Zi does not produce .pdb files for static
REM libs; use /Z7 instead to embed CodeView info in the .obj files so that MSVC link.exe can
REM pull it into hatngrpcclient.pdb at DLL link time.  Set by the caller; empty by default.
IF NOT DEFINED DEBUG_INFO_CMAKE SET DEBUG_INFO_CMAKE=

cmake %ARCH_CMAKE% %COMPILER_CMAKE% %LINKER_CMAKE% %DEBUG_INFO_CMAKE% -DCMAKE_BUILD_TYPE=%GRPC_BUILD_TYPE% -DCMAKE_INSTALL_PREFIX=%DEPS_PREFIX% ^
        -DCMAKE_PREFIX_PATH=%SYSTEM_DEPS_PREFIX% ^
        -DgRPC_INSTALL=ON ^
        -DgRPC_BUILD_TESTS=OFF ^
        -DgRPC_BUILD_CSHARP_EXT=OFF ^
        -DCMAKE_CXX_STANDARD=17 ^
        -Dprotobuf_INSTALL=ON ^
        -DABSL_ENABLE_INSTALL=ON ^
        -Dre2_INSTALL=ON ^
        -Dutf8_range_INSTALL=ON ^
        -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF ^
        -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF ^
        -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF ^
        -DgRPC_CARES_PROVIDER=package ^
        -DgRPC_SSL_PROVIDER=package ^
        "-Dc-ares_DIR=%SYSTEM_DEPS_PREFIX%/lib/cmake/c-ares" ^
        "-DOPENSSL_INCLUDE_DIR=%SYSTEM_DEPS_PREFIX%/include" ^
        "-DOPENSSL_CRYPTO_LIBRARY=%SYSTEM_DEPS_PREFIX%/lib/libcrypto.lib" ^
        "-DOPENSSL_SSL_LIBRARY=%SYSTEM_DEPS_PREFIX%/lib/libssl.lib" ^
        %folder%
if %errorlevel% neq 0 exit %errorlevel%

IF NOT DEFINED CMAKE_MSVC_GENERATOR (

cmake --build . --target install --config %GRPC_BUILD_TYPE% -- -j %BUILD_WORKERS%
if %errorlevel% neq 0 exit %errorlevel%

) ELSE (

cmake --build . --target install --config %GRPC_BUILD_TYPE% -- /m:1 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
if %errorlevel% neq 0 exit %errorlevel%

)

cd %WORKING_DIR%
