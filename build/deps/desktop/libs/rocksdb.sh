export lib_name=rocksdb

export folder=$src_dir/$lib_name
export repo_path=https://github.com/facebook/$lib_name

source $scripts_root/scripts/clonegit.sh

cd $lib_build_dir

set GFLAGS_INCLUDE=$root_dir/include
set GFLAGS_LIB_RELEASE=$root_dir/lib/gflags_static.lib
set LZ4_INCLUDE=$root_dir/include
set LZ4_LIB_RELEASE=$root_dir/lib/liblz4_static.lib

if ! [[ "$platform" == "windows" ]]; then

    export CXXFLAGS=-fPIC
    
    set GFLAGS_INCLUDE=$root_dir/include
    set GFLAGS_LIB_RELEASE=$root_dir/lib/libgflags.a
    set LZ4_INCLUDE=$root_dir/include
    set LZ4_LIB_RELEASE=$root_dir/lib/liblz4.a
fi

cmake -G "$cmake_gen_prefix Makefiles" \
        -DCMAKE_SH="CMAKE_SH-NOTFOUND" \
        -DCMAKE_INSTALL_PREFIX=$root_dir \
        -DCMAKE_BUILD_TYPE=Release \
        -DWITH_TESTS=0 \
        -DFAIL_ON_WARNINGS=0 \
        -DWITH_GFLAGS=1 \
        -DWITH_LZ4=1 \
        -DROCKSDB_INSTALL_ON_WINDOWS=1 \
	    -DMINGW_NO_POSIX_THREAD=1 \
        $folder 
$make_tool install -j$build_workers install

unset LDFLAGS
unset CXXFLAGS
