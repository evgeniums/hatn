if [ -d "$folder" ]; then
    echo  $folder already exists, skip downloading
else
    if [ -f "$downloads_dir/$archive" ]; then
        echo  $archive already downloaded
    else
        echo Downloading $archive ...
        curl -L $download_link --output $downloads_dir/$archive
    fi
    echo "Unpacking $archive ..."
    7z x $downloads_dir/$archive -o$src_dir
fi

export lib_build_dir=$build_dir/$lib_name
if [ -d "$lib_build_dir" ]; then
    rm -rf $lib_build_dir 
fi
mkdir -p $lib_build_dir

