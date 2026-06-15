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

@ECHO ON

cmake -G Ninja -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_C_COMPILER_TARGET=%CLANG_TARGET% -DCMAKE_CXX_COMPILER_TARGET=%CLANG_TARGET% "-DCMAKE_LINKER=%MSVC_LINKER%" -DDEV_MODULE=%BUILD_MODULE% -DOPENSSL_ROOT_DIR=%OPENSSL_ROOT_DIR% -DBUILD_PLUGINS="%HATN_PLUGINS%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DINSTALL_DEV=%INSTALL_DEV% -DBUILD_STATIC=%BUILD_STATIC% -DENABLE_TRANSLATIONS=1 -DPYTHON_PATH=%PYTHON_PATH% -DGETTEXT_PATH=%GETTEXT_PATH% %USE_BOOST_COMPILER% "-DGRPC_DEPS_ROOT=%GRPC_DEPS_ROOT%" %PROJECT_PATH%
if %errorlevel% neq 0 exit %errorlevel%
cmake --build . --target install -j %BUILD_WORKERS%
if %errorlevel% neq 0 exit %errorlevel%

@ECHO OFF

xcopy install %INSTALL_DIR%\ /s /q /e /y
if %errorlevel% neq 0 exit %errorlevel%
