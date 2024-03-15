@ECHO OFF

call "%MSVCARGS%" %MSVC_ARCH%
if %errorlevel% neq 0 exit %errorlevel%

echo "BUILDS_ROOT=BUILDS_ROOT=%builds_root%"

@ECHO ON
cmake -A %MSVC_BUILD_ARCH% -T %MSVC_TOOLSET% -DDEV_MODULE=%BUILD_MODULE% -DOPENSSL_ROOT_DIR=%OPENSSL_ROOT_DIR% -DBUILD_PLUGINS="%HATN_PLUGINS%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DINSTALL_DEV=%INSTALL_DEV% -DBUILD_STATIC=%BUILD_STATIC% -DENABLE_TRANSLATIONS=1 -DPYTHON_PATH=%PYTHON_PATH% -DGETTEXT_PATH=%GETTEXT_PATH% %PROJECT_PATH%
if %errorlevel% neq 0 exit %errorlevel%
cmake --build . --target install --config %BUILD_TYPE% -- /m:4 /p:UseMultiToolTask=true /p:MultiProcMaxCount=%BUILD_WORKERS% /fileLogger
@ECHO OFF
if %errorlevel% neq 0 exit %errorlevel%
xcopy install %INSTALL_DIR%\ /s /q /e /y
if %errorlevel% neq 0 exit %errorlevel%
