ECHO OFF

IF EXIST  ..\..\..\hatn.src (
    ECHO Do not run build scripts in source directory!
    ECHO Please, invoke %0 in some other working directory.
    EXIT /b 1
)

SET TARGET=%1
IF "%TARGET%"=="" GOTO :error

SET SCRIPTS_ROOT=%~dp0

SET TARGETS=scripts\%TARGET%

IF NOT EXIST %TARGETS% mkdir %TARGETS%

SET PLATFORM=%2
IF "%PLATFORM%"=="" (
    SET PLATFORM=mingw
    SET mingw_and_msvc=yes
)

SET ADDRESS_SPACE=x86
IF "%PLATFORM%"=="mingw" SET ADDRESS_SPACE=x86_64

:copy_files

SET TARGET_PATH=%TARGETS%\%PLATFORM%-%ADDRESS_SPACE%
IF NOT EXIST %TARGET_PATH% mkdir %TARGET_PATH%

IF NOT EXIST "%PLATFORM%-env.bat" copy "%SCRIPTS_ROOT%%PLATFORM%\env.bat" "%PLATFORM%-env.bat"
IF NOT EXIST "%PLATFORM%-x86-env.bat" (
    IF NOT "%PLATFORM%"=="mingw" copy "%SCRIPTS_ROOT%%PLATFORM%\x86-env.bat" "%PLATFORM%-x86-env.bat"
)
IF NOT EXIST "%PLATFORM%-x86_64-env.bat" copy "%SCRIPTS_ROOT%%PLATFORM%\x86_64-env.bat" "%PLATFORM%-x86_64-env.bat"
IF NOT EXIST "clean.bat" copy "%SCRIPTS_ROOT%\clean.bat" "clean.bat"
 
(
  ECHO @ECHO OFF
  ECHO SET PLATFORM=%PLATFORM%
  ECHO SET INITIAL_DIR=%%CD%%
  ECHO cd %SCRIPTS_ROOT%scripts
  ECHO call build.bat
  ECHO cd %%INITIAL_DIR%%
) > %TARGET_PATH%\build.bat 


ECHO @ECHO OFF > %TARGET_PATH%\all_%ADDRESS_SPACE%.bat
FOR /f %%i IN ('dir /b %SCRIPTS_ROOT%configurations') DO (
  call :each_target %%i
)
ECHO IF "%%1"=="exit" exit >> %TARGET_PATH%\all_%ADDRESS_SPACE%.bat

IF "%PLATFORM%"=="mingw" (
    IF "%ADDRESS_SPACE%"=="x86" (
    SET ADDRESS_SPACE=x86_64
    GOTO :copy_files  
  )
  IF "%mingw_and_msvc%"=="yes" (  
    SET PLATFORM=msvc
    SET ADDRESS_SPACE=x86
    GOTO :copy_files
  )
) ELSE (    
    IF "%ADDRESS_SPACE%"=="x86" (
    SET ADDRESS_SPACE=x86_64
    GOTO :copy_files  
  )
)

(
ECHO @ECHO OFF
ECHO cd mingw
ECHO ECHO Building all in mingw...
ECHO call all_x86.bat
ECHO cd mingw-x86_64
ECHO ECHO Building all in mingw x64...
ECHO call all_x64.bat
ECHO cd ..
ECHO cd msvc
ECHO ECHO Building all in msvc...
ECHO call all_x86.bat
ECHO cd msvc-x86_64
ECHO ECHO Building all in msvc x86_64...
ECHO call all_x86_64.bat
ECHO cd ..
ECHO ECHO Done
) > %TARGETS%\all.bat

GOTO:EOF

:each_target
   SET TARGET_CFG=%~n1
   SET TARGET_CFG=%TARGET_CFG%-%ADDRESS_SPACE%
   SET TARGET_NAME=%PLATFORM%-%TARGET%-%TARGET_CFG%
   SET TARGET_SCRIPT=%TARGET_PATH%\%TARGET_CFG%.bat   
   ECHO call %TARGET_CFG% >> %TARGET_PATH%\all_%ADDRESS_SPACE%.bat   
   echo Adding configuration %TARGET_NAME%
  (     
    ECHO @ECHO OFF
    ECHO SET WORKING_DIR=%%~dp0..\..\..
    ECHO SET SCRIPTS_ROOT=%SCRIPTS_ROOT%
    ECHO SET BUILD_MODULE=%TARGET%
    ECHO SET ADDRESS_SPACE=%ADDRESS_SPACE%
    ECHO SET TARGET=%~n1
    IF "%ADDRESS_SPACE%"=="x86_64" (ECHO SET BUILD_64=1) ELSE (ECHO SET BUILD_64=0) 
    ECHO ECHO ***************************************
    ECHO ECHO Building %TARGET_NAME%
    ECHO ECHO *************************************** 
    ECHO call %%~dp0\build.bat
  ) > %TARGET_SCRIPT%     
GOTO:EOF

:error
echo "Usage: %0 <lib> [msvc|mingw]"