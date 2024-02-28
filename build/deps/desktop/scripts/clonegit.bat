SET oldpwd=%cd%
IF EXIST %folder% (
    ECHO  %folder% already exists, just pull
    cd %folder%
    git pull --recurse-submodules
    cd %oldpwd%    
) else (
    cd %SRC%
    git clone --recurse-submodules %repo_path%
    cd %oldpwd%    
    IF %errorlevel% NEQ 0 (
        ECHO Failed to clone repository
        EXIT /b %errorlevel%
    )    
)

SET build_dir=%BUILD_FOLDER%\%lib_name%
IF EXIST %build_dir% (
    rmdir /s /q %build_dir% 
)
mkdir %build_dir%

cd %folder%
