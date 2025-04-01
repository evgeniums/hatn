@ECHO OFF

call "%MSVCARGS%" %MSVC_ARCH%
if %errorlevel% neq 0 exit %errorlevel%

echo "BUILDS_ROOT=%builds_root%"

IF DEFINED BOOST_COMPILER (
  SET USE_BOOST_COMPILER="-DBoost_COMPILER=%BOOST_COMPILER%"
)

IF NOT DEFINED CMAKE_MSVC_GENERATOR (

@ECHO ON

cmake -G Ninja -DDEV_MODULE=%BUILD_MODULE% -DOPENSSL_ROOT_DIR=%OPENSSL_ROOT_DIR% -DBUILD_PLUGINS="%HATN_PLUGINS%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DINSTALL_DEV=%INSTALL_DEV% -DBUILD_STATIC=%BUILD_STATIC% -DENABLE_TRANSLATIONS=1 -DPYTHON_PATH=%PYTHON_PATH% -DGETTEXT_PATH=%GETTEXT_PATH% %USE_BOOST_COMPILER% %PROJECT_PATH%
if %errorlevel% neq 0 exit %errorlevel%
cmake --build . --target install -j %BUILD_WORKERS%
if %errorlevel% neq 0 exit %errorlevel%

@ECHO OFF

) ELSE (

@ECHO ON

cmake -G "%CMAKE_MSVC_GENERATOR%" -A %MSVC_BUILD_ARCH% -T %MSVC_TOOLSET% -DDEV_MODULE=%BUILD_MODULE% -DOPENSSL_ROOT_DIR=%OPENSSL_ROOT_DIR% -DBUILD_PLUGINS="%HATN_PLUGINS%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DINSTALL_DEV=%INSTALL_DEV% -DBUILD_STATIC=%BUILD_STATIC% -DENABLE_TRANSLATIONS=1 -DPYTHON_PATH=%PYTHON_PATH% -DGETTEXT_PATH=%GETTEXT_PATH% %USE_BOOST_COMPILER% %PROJECT_PATH%
if %errorlevel% neq 0 exit %errorlevel%
cmake --build . --target install --config %BUILD_TYPE% -- /m:1 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
if %errorlevel% neq 0 exit %errorlevel%

@ECHO OFF
)

xcopy install %INSTALL_DIR%\ /s /q /e /y
if %errorlevel% neq 0 exit %errorlevel%
