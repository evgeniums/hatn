dir_suffix=$toolchain
export make_tool=make
export cmake_gen_prefix=Unix

if [[ "$toolchain" == "mingw" ]]; then 
    dir_suffix=mingw-x86
    export make_tool=mingw32-make
    export cmake_gen_prefix=MinGW
fi

if [[ "$toolchain" == "mingw64" ]]; then 
    dir_suffix=mingw-x86_64
    export make_tool=mingw32-make
    export cmake_gen_prefix=MinGW
fi

export build_dir=${working_dir}/build-$dir_suffix
export root_dir=${working_dir}/root-$dir_suffix
export downloads_dir=${working_dir}/downloads
export src_dir=${working_dir}/src
