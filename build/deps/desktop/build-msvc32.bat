@ECHO OFF

SET KEEP_PATH=%PATH%
SET _current_path=%CD%

IF NOT EXIST deps mkdir deps
cd deps

SET HATN_ARCH="x86"

SET SCRIPTS_ROOT=%~dp0

CALL %SCRIPTS_ROOT%\scripts\runner.bat

SET PATH=%KEEP_PATH%
cd %_current_path%
