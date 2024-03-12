SET KEEP_PATH=%PATH%

SET _COMPILER=%PLATFORM%

IF "%BUILD_64%"=="1" (
    IF EXIST %WORKING_DIR%\%PLATFORM%-x86_64-env.bat call %WORKING_DIR%\%PLATFORM%-x86_64-env.bat
) ELSE (
    IF EXIST %WORKING_DIR%\%PLATFORM%-x86-env.bat call %WORKING_DIR%\%PLATFORM%-x86-env.bat
)
IF EXIST %WORKING_DIR%\%PLATFORM%-env.bat call %WORKING_DIR%\%PLATFORM%-env.bat
call %SCRIPTS_ROOT%scripts\paths.bat
call %SCRIPTS_ROOT%%PLATFORM%\config.bat
call %SCRIPTS_ROOT%configurations\%TARGET%.bat

SET PROJECT_PATH=%SOURCES_ROOT%
SET PROJECT_FULL=%PLATFORM%-%BUILD_MODULE%-%BUILD_TYPE%
IF "%BUILD_STATIC%"=="1" (
SET PROJECT_FULL=%PROJECT_FULL%-static
) ELSE (
SET PROJECT_FULL=%PROJECT_FULL%-shared
)

IF "%INSTALL_DEV%"=="1" SET PROJECT_FULL=%PROJECT_FULL%-dev
IF "%BUILD_64%"=="1" (
SET PROJECT_FULL=%PROJECT_FULL%-x86_64
) ELSE (
SET PROJECT_FULL=%PROJECT_FULL%-x86
)

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

cd %WORKING_DIR%

IF "%PREPARE_TESTS%"=="1" (
    ECHO Preparing tests script %_COMPILER% %WORKING_DIR%\build\run-tests.bat ...
    SETLOCAL ENABLEDELAYEDEXPANSION
    IF "%_COMPILER%"=="msvc" (
        SET TEST_DIR=%BUILD_DIR%\test\%BUILD_TYPE%
    ) ELSE (
        SET TEST_DIR=%BUILD_DIR%\test
    )
    echo 
    IF "%HATN_TEST_NAME%"=="" (
        ECHO "Testing all"
		SET CTEST_ARGS=-L ALL
    )  ELSE (
        ECHO "Testing %HATN_TEST_NAME% only"
		
		SET TMP_NAME=%HATN_TEST_NAME%
		echo.!TMP_NAME!|findstr /C:"/" >nul 2>&1
		if not errorlevel 1 (
		   SET CTEST_ARGS=-L CASE -R %HATN_TEST_NAME%
		) else (
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
        ECHO ECHO Running test script
        ECHO SET CURRENT_DIR=%%CD%%
        ECHO SET "PATH=%PATH%;!TEST_DIR!"
        ECHO cd !TEST_DIR!
        ECHO ctest -C !CTEST_CONFIGURATION! !CTEST_ARGS! --verbose --test-dir %BUILD_DIR%/test
        ECHO cd %%CURRENT_DIR%%
    ) > build\run-tests.bat
    
    REM (
        REM ECHO ECHO Running test script
        REM ECHO SET CURRENT_DIR=%%CD%%
        REM ECHO SET "PATH=%PATH%;!TEST_DIR!"
        REM ECHO cd !TEST_DIR!
        REM ECHO hatnlibs-test.exe --log_level=test_suite !MEMORY_LEAKS! !RUN_TEST!
        REM ECHO cd %%CURRENT_DIR%%
    REM ) > build\run-tests-manual.bat
)

SET PATH=%KEEP_PATH%
