@ECHO OFF
REM Build gRPC (+ bundled protobuf / abseil / re2 / utf8_range) with clang-cl for x86_64.
REM
REM gRPC installs into  root-clang-cl-<COMPILER_VERSION>-x86_64  (sibling of the MSVC root)
REM so that hatngrpcclient links a fully clang-cl-ABI-compatible gRPC stack.
REM c-ares and OpenSSL are left as MSVC builds (pure-C ABI, safe to reuse).
REM
REM Optional overrides (set before calling this script or in the working-dir env):
REM   MSVC_ROOT            VS install directory
REM                        default: "C:\Program Files\Microsoft Visual Studio\18\Community"
REM   LLVM_ROOT            LLVM install directory
REM                        default: MSVC bundled Clang (%MSVC_ROOT%\VC\Tools\Llvm\x64)
REM   COMPILER_VERSION     MSVC toolset version used to name the deps root, e.g. 14.51
REM                        default: value from config.bat (14.51)
REM   GRPC_VERSION         gRPC tag to build, e.g. 1.78.1
REM                        default: value from config.bat
REM   GRPC_BUILD_TYPE      cmake build type: Release (default) or RelWithDebInfo.
REM                        Use RelWithDebInfo to get debug symbols while keeping /MD.
REM   DEPS_UNIVERSAL_ROOT  Universal deps base; both the MSVC root (c-ares/OpenSSL) and the
REM                        clang-cl gRPC root are derived from it.
REM                        If unset, roots are located inside the local  deps\  subfolder.
REM   SYSTEM_DEPS_PREFIX   Explicit path to the MSVC deps root (c-ares, OpenSSL).
REM                        Auto-derived from DEPS_UNIVERSAL_ROOT / COMPILER_VERSION when unset.

SET KEEP_PATH=%PATH%
SET _current_path=%CD%

SET HATN_ARCH=x86_64
SET COMPILER=clang-cl
SET TOOLCHAIN_LINE=-
SET LLVM_HOST_DIR=x64
SET CLANG_TARGET=x86_64-pc-windows-msvc
SET MSVC_ARCH=x64

SET SCRIPTS_ROOT=%~dp0

REM --- Version defaults: COMPILER_VERSION, GRPC_VERSION, BUILD_WORKERS ---
CALL %SCRIPTS_ROOT%config.bat

IF NOT EXIST deps mkdir deps
cd deps
SET WORKING_DIR=%CD%

REM --- Derive DEPS_PREFIX (clang-cl gRPC install root) and BUILD_FOLDER ---
CALL %SCRIPTS_ROOT%paths.bat
ECHO "DEPS_PREFIX  (clang-cl gRPC install): %DEPS_PREFIX%"

REM --- Derive SYSTEM_DEPS_PREFIX: the MSVC root containing c-ares and OpenSSL ---
IF "%SYSTEM_DEPS_PREFIX%"=="" (
    IF "%DEPS_UNIVERSAL_ROOT%"=="" (
        SET "SYSTEM_DEPS_PREFIX=%WORKING_DIR%\root-msvc-%COMPILER_VERSION%-%HATN_ARCH%"
    ) ELSE (
        SET "SYSTEM_DEPS_PREFIX=%DEPS_UNIVERSAL_ROOT%\root-msvc-%COMPILER_VERSION%-%HATN_ARCH%"
    )
)
ECHO "SYSTEM_DEPS_PREFIX (c-ares/OpenSSL): %SYSTEM_DEPS_PREFIX%"

REM --- MSVC and LLVM environment setup ---
IF "%MSVC_ROOT%"=="" SET "MSVC_ROOT=C:\Program Files\Microsoft Visual Studio\18\Community"
IF NOT DEFINED LLVM_ROOT SET "LLVM_ROOT=%MSVC_ROOT%\VC\Tools\Llvm\%LLVM_HOST_DIR%"
SET "PATH=%LLVM_ROOT%\bin;%PATH%"
ECHO "Using LLVM_ROOT=%LLVM_ROOT%"

CALL "%MSVC_ROOT%\VC\Auxiliary\Build\vcvarsall.bat" %MSVC_ARCH%
if %errorlevel% neq 0 exit %errorlevel%

REM --- Locate MSVC link.exe (must be after vcvarsall so it appears in PATH) ---
FOR %%i IN (link.exe) DO SET "MSVC_LINKER=%%~$PATH:i"
ECHO "Using linker: %MSVC_LINKER%"

REM --- cmake args for clang-cl (compiler + linker) ---
SET "COMPILER_CMAKE=-DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_C_COMPILER_TARGET=%CLANG_TARGET% -DCMAKE_CXX_COMPILER_TARGET=%CLANG_TARGET%"
SET LINKER_CMAKE="-DCMAKE_LINKER=%MSVC_LINKER%"

REM With clang-cl, /Zi does not produce .pdb files for static libs; use /Z7 so debug info
REM is embedded in the .obj files.  MSVC link.exe then merges it all into hatngrpcclient.pdb
REM when the DLL is linked — no separate gRPC .pdb files required.
IF "%GRPC_BUILD_TYPE%"=="RelWithDebInfo" (
    SET DEBUG_INFO_CMAKE="-DCMAKE_C_FLAGS_RELWITHDEBINFO=/O2 /Z7 /DNDEBUG" "-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=/O2 /Z7 /DNDEBUG"
)

REM --- Force Ninja; MSBuild generator is incompatible with clang-cl ---
SET CMAKE_MSVC_GENERATOR=

REM --- Create output directories ---
IF NOT EXIST %BUILD_FOLDER% mkdir %BUILD_FOLDER%
IF NOT EXIST %DOWNLOADS%    mkdir %DOWNLOADS%
IF NOT EXIST %SRC%          mkdir %SRC%
IF NOT EXIST %DEPS_PREFIX%  mkdir %DEPS_PREFIX%

ECHO.
ECHO === Building gRPC %GRPC_VERSION% with clang-cl for %HATN_ARCH% ===
ECHO.

CALL %SCRIPTS_ROOT%libs\grpc.bat
if %errorlevel% neq 0 exit %errorlevel%

SET PATH=%KEEP_PATH%
cd %_current_path%
