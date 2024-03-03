IF "%COMPILER%"=="" SET COMPILER=msvc
echo "COMPILER=%COMPILER%"
SET TOOLCHAIN_LINE=-
CALL %SCRIPTS_ROOT%\config.bat

SET WORKING_DIR=%CD%

IF EXIST env-%COMPILER%.bat (
CALL env-%COMPILER%.bat
) ELSE (
copy %SCRIPTS_ROOT%\sample-env\env-%COMPILER%.bat env-%COMPILER%.bat
ECHO "**********************"
ECHO "Please, view and edit env-%COMPILER%.bat in working directory to setup environment"
ECHO "**********************"
CALL env-%COMPILER%.bat
)

WHERE curl > nul 2>&1
IF %errorlevel% NEQ 0 (
    ECHO curl is not found, please install curl and add it to PATH
    EXIT /b %errorlevel%
)

WHERE 7z > nul 2>&1
IF %errorlevel% NEQ 0 (
    ECHO 7z is not found, please install 7z and add it to PATH
    EXIT /b %errorlevel%
)

IF EXIST  ..\..\..\hatn.src (
    ECHO Do not run build scripts in source directory!
    ECHO Please, invoke %0 in some other working directory.
    EXIT /b 1
)

CALL %SCRIPTS_ROOT%\scripts\prepare-%COMPILER%.bat
CALL %SCRIPTS_ROOT%\paths.bat

IF NOT EXIST %BUILD_FOLDER% (
    mkdir %BUILD_FOLDER% 
)

IF NOT EXIST %DOWNLOADS% (
    mkdir %DOWNLOADS% 
)

IF NOT EXIST %SRC% (
    mkdir %SRC% 
)

IF NOT EXIST %DEPS_PREFIX% (
    mkdir %DEPS_PREFIX% 
)

FOR %%f IN (%DEP_LIBS%) do (
    ECHO Building %%f ...
    CALL %DEPS_ROOT%\%%f.bat        
)
    
