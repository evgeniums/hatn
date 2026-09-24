@ECHO OFF

IF DEFINED CMAKE_MSVC_GENERATOR (
ECHO "run.bat: clang-cl profile supports only the Ninja generator, please unset CMAKE_MSVC_GENERATOR"
exit 1
)

call "%MSVCARGS%" %MSVC_ARCH%
if %errorlevel% neq 0 exit %errorlevel%

ECHO "run.bat: BUILDS_ROOT=%builds_root%, PROJECT_PATH=%PROJECT_PATH%, running in %CD%"

IF DEFINED BOOST_COMPILER (
  SET USE_BOOST_COMPILER="-DBoost_COMPILER=%BOOST_COMPILER%"
)

REM Link with MSVC link.exe (not lld-link next to clang-cl) so that version-specific
REM MSVC STL satellite libs (__std_* helpers in msvcprt.lib) match the deps toolset.
SET MSVC_LINKER=
FOR %%i IN (link.exe) DO SET "MSVC_LINKER=%%~$PATH:i"
ECHO "run.bat: using linker %MSVC_LINKER%"

REM Pin the archiver to llvm-lib.exe (MSVC-compatible COFF librarian, bundled next to
REM clang-cl in %LLVM_ROOT%\bin). Without this, CMake's auto-detected CMAKE_AR can resolve
REM to llvm-ar.exe (GNU/Unix archive format) instead, which produces a .lib that MSVC's
REM link.exe rejects with "LNK4003: invalid library format; library ignored" - this only
REM shows up for libraries actually built STATIC under this profile (e.g. the client lib,
REM which whitemclient/CMakeLists.txt forces to STATIC in Release regardless of BUILD_STATIC);
REM SHARED libs are unaffected since their .lib is an import lib written by link.exe itself.
SET LLVM_LIB=
FOR %%i IN (llvm-lib.exe) DO SET "LLVM_LIB=%%~$PATH:i"
ECHO "run.bat: using archiver %LLVM_LIB%"

@ECHO ON

cmake -G Ninja -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_C_COMPILER_TARGET=%CLANG_TARGET% -DCMAKE_CXX_COMPILER_TARGET=%CLANG_TARGET% "-DCMAKE_LINKER=%MSVC_LINKER%" "-DCMAKE_AR=%LLVM_LIB%" -DDEV_MODULE=%BUILD_MODULE% -DOPENSSL_ROOT_DIR=%OPENSSL_ROOT_DIR% -DBUILD_PLUGINS="%HATN_PLUGINS%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DINSTALL_DEV=%INSTALL_DEV% -DBUILD_STATIC=%BUILD_STATIC% -DENABLE_TRANSLATIONS=1 -DPYTHON_PATH=%PYTHON_PATH% -DGETTEXT_PATH=%GETTEXT_PATH% %USE_BOOST_COMPILER% "-DGRPC_DEPS_ROOT=%GRPC_DEPS_ROOT%" %PROJECT_PATH%
if %errorlevel% neq 0 exit %errorlevel%
cmake --build . --target install -j %BUILD_WORKERS%
if %errorlevel% neq 0 exit %errorlevel%

@ECHO OFF

xcopy install %INSTALL_DIR%\ /s /q /e /y
if %errorlevel% neq 0 exit %errorlevel%
