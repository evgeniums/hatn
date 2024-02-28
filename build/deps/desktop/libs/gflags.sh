export lib_name=gflags

export folder=$src_dir/$lib_name
export repo_path=https://github.com/gflags/$lib_name

source $scripts_root/scripts/clonegit.sh

cd $lib_build_dir

if ! [[ "$platform" == "windows" ]]; then
    export CXXFLAGS=-fPIC
fi

cmake -G "$cmake_gen_prefix Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND" -DCMAKE_INSTALL_PREFIX=$root_dir -DCMAKE_BUILD_TYPE=Release -DREGISTER_INSTALL_PREFIX=0 $folder 
$make_tool -j$build_workers install
