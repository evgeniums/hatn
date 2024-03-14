export lib_name=rapidjson

export folder=$src_dir/$lib_name
export repo_path=https://github.com/Tencent/$lib_name

source $scripts_root/scripts/clonegit.sh

cd $folder
patch_file=$scripts_root/libs/rapidjson.patch
if ! patch -R -p0 -s -f --dry-run <$patch_file > /dev/null; then
  echo "rapidjson distro must be patched with $patch_file"
  patch -p0 <$patch_file
  echo "rapidjson patched"
fi

cd $lib_build_dir

if ! [[ "$platform" == "windows" ]]; then
    export CXXFLAGS=-fPIC
fi

cmake -G "$cmake_gen_prefix Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND" -DCMAKE_INSTALL_PREFIX=$root_dir -DCMAKE_BUILD_TYPE=Release -DREGISTER_INSTALL_PREFIX=0 -DRAPIDJSON_BUILD_EXAMPLES=Off -DRAPIDJSON_BUILD_TESTS=Off -DRAPIDJSON_BUILD_DOC=Off $folder  
$make_tool -j$build_workers install
