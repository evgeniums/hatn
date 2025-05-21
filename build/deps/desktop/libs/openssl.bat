SET lib_name=openssl-%OPENSSL_VERSION%

rem https://codeload.github.com/openssl/openssl/zip/refs/tags/openssl-3.2.1

SET archive=%lib_name%.zip
SET download_link=https://codeload.github.com/openssl/openssl/zip/refs/tags/%lib_name%

echo "Downloading %download_link%"

SET folder=%SRC%\openssl-%lib_name%

CALL %SCRIPTS_ROOT%/scripts/downloadandunpack.bat

IF "%HATN_ARCH%"=="x86" SET openssl_target=VC-WIN
IF "%HATN_ARCH%"=="x86_64" SET openssl_target=VC-WIN64A
IF "%HATN_ARCH%"=="arm64" SET openssl_target=VC-WIN64-ARM

cd %build_dir%
perl %folder%\Configure no-asm --prefix=%DEPS_PREFIX% --openssldir=%DEPS_PREFIX%\var\lib\openssl %openssl_target%
nmake install

cd %WORKING_DIR%