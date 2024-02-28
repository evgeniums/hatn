SET openssl_underscore_version=%OPENSSL_VERSION%
SETLOCAL ENABLEDELAYEDEXPANSION
SET openssl_underscore_version=%openssl_underscore_version:.=_%
SETLOCAL DISABLEDELAYEDEXPANSION

SET lib_name=openssl-%OPENSSL_VERSION%

rem https://codeload.github.com/openssl/openssl/zip/refs/tags/openssl-3.2.1

SET archive=%lib_name%.zip
SET download_link=https://codeload.github.com/openssl/openssl/zip/refs/tags/%lib_name%

echo "Downloading %download_link%"

SET folder=%SRC%\openssl-%lib_name%

CALL %SCRIPTS_ROOT%/scripts/downloadandunpack.bat

IF %ADDRESS_MODEL%==64 SET openssl_postfix=A

cd %build_dir%
perl %folder%\Configure no-asm --prefix=%DEPS_PREFIX% --openssldir=%DEPS_PREFIX%\var\lib\openssl VC-WIN%ADDRESS_MODEL%%openssl_postfix%
nmake install

cd %WORKING_DIR%