SET KEEP_PATH=%PATH%

SET _COMPILER=%PLATFORM%

IF EXIST %WORKING_DIR%\%PLATFORM%-%ADDRESS_SPACE%-env.bat call %WORKING_DIR%\%PLATFORM%-%ADDRESS_SPACE%-env.bat

IF EXIST %WORKING_DIR%\%PLATFORM%-env.bat call %WORKING_DIR%\%PLATFORM%-env.bat
call %SCRIPTS_ROOT%scripts\paths.bat
call %SCRIPTS_ROOT%%PLATFORM%\config.bat
call %SCRIPTS_ROOT%configurations\%TARGET%.bat

SET PROJECT_PATH=%SRC_ROOT%
SET PROJECT_FULL=%PLATFORM%-%BUILD_MODULE%-%BUILD_TYPE%

IF "%BUILD_STATIC%"=="1" (
SET PROJECT_FULL=%PROJECT_FULL%-static
) ELSE (
SET PROJECT_FULL=%PROJECT_FULL%-shared
)

IF "%INSTALL_DEV%"=="1" SET PROJECT_FULL=%PROJECT_FULL%-dev

SET PROJECT_FULL=%PROJECT_FULL%-%ADDRESS_SPACE%

SET BUILD_ROOT=%BUILDS_ROOT%\builds
SET INSTALL_SANDBOX=%BUILDS_ROOT%\install

SET BUILD_DIR=%BUILD_ROOT%\%PROJECT_FULL%
SET INSTALL_DIR=%INSTALL_SANDBOX%\%PROJECT_FULL%

IF EXIST %BUILD_DIR% rmdir /Q /S %BUILD_DIR%
IF EXIST %INSTALL_DIR% rmdir /Q /S %INSTALL_DIR%
mkdir %BUILD_DIR%

SET CURRENT_DIR=%CD%
cd %BUILD_DIR%

SET PATH=%BOOST_ROOT%\lib;%DEPS_ROOT%\bin;%PATH%
call %SCRIPTS_ROOT%%PLATFORM%\run.bat

IF "%PREPARE_TESTS%"=="1" (
    ECHO "Preparing tests script %WORKING_DIR%\run-tests.bat"
    SETLOCAL ENABLEDELAYEDEXPANSION
    IF "%_COMPILER%"=="msvc" (
        SET TEST_DIR=%BUILD_DIR%\test\%BUILD_TYPE%
    ) ELSE (
        SET TEST_DIR=%BUILD_DIR%\test
    )
    IF "%HATN_TEST_NAME%"=="" (
        ECHO "Will run ALL tests"
		SET CTEST_ARGS=-L ALL
    )  ELSE (		
		SET TMP_NAME=%HATN_TEST_NAME%
		echo.!TMP_NAME!|findstr /C:"/" >nul 2>&1
		if not errorlevel 1 (
		   ECHO "Will run test SUITE %HATN_TEST_NAME%"
		   SET CTEST_ARGS=-L CASE -R %HATN_TEST_NAME%
		) else (
		   ECHO "Will run test CASSE %HATN_TEST_NAME%"
		   SET CTEST_ARGS=-L SUITE -R %HATN_TEST_NAME%
		)		
    )
    IF "%BUILD_TYPE%"=="debug" (
        SET MEMORY_LEAKS=--detect_memory_leaks=0
		SET CTEST_CONFIGURATION=Debug
    ) ELSE (
		SET CTEST_CONFIGURATION=Release
	)                          	
	
    (
        @ECHO OFF
        ECHO ECHO Running test script
		ECHO IF EXIST %BUILDS_ROOT%\result-xml rmdir /Q /S %BUILDS_ROOT%\result-xml
		ECHO mkdir %BUILDS_ROOT%\result-xml
        ECHO SET CURRENT_DIR=%%CD%%
        ECHO SET "PATH=%PATH%;!TEST_DIR!"
        ECHO cd !TEST_DIR!
		@ECHO ON
        ECHO ctest -C !CTEST_CONFIGURATION! !CTEST_ARGS! --verbose --test-dir %BUILD_DIR%/test
		@ECHO OFF
        ECHO cd %%CURRENT_DIR%%
    ) > %WORKING_DIR%\run-tests.bat 
)

SET PATH=%KEEP_PATH%
