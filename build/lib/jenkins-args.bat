@ECHO OFF
SET HATN_LIB=%1
SET HATN_COMPILER=%2
SET HATN_ARCH=%3
SET HATN_BUILD=%4
SET HATN_LINK=%5
SET HATN_PLUGINS=%6
SET mypath=%~dp0

call %mypath%\jenkins.bat
