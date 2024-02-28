if [ -d "$folder" ]; then
    echo  $folder already exists, just pull
    cd $folder
    git pull --recurse-submodules
    cd -
else
    cd $src_dir
    git clone --recurse-submodules $repo_path
    cd -
fi

export lib_build_dir=$build_dir/$lib_name
if [ -d "$lib_build_dir" ]; then
    rm -rf $lib_build_dir 
fi
mkdir -p $lib_build_dir

