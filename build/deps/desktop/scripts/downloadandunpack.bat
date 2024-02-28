IF EXIST %folder% (
    ECHO  %folder% already exists, skip downloading
) else (
    IF EXIST %DOWNLOADS%\%archive% (
        ECHO  %archive% already downloaded
    ) ELSE (
        ECHO Downloading %archive% ...
        curl -L %download_link% --output %DOWNLOADS%/%archive%
        IF %errorlevel% NEQ 0 (
            ECHO Failed to download boost
            EXIT /b %errorlevel%
        )        
    )
    7z x %DOWNLOADS%\%archive% -o%SRC%
    IF %errorlevel% NEQ 0 (
        ECHO Failed to unpack boost
        EXIT /b %errorlevel%
    )    
)

SET build_dir=%BUILD_FOLDER%\%lib_name%
IF EXIST %build_dir% (
    rmdir /s /q %build_dir% 
)
mkdir %build_dir%

cd %folder%
