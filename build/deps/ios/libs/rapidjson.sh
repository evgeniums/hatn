export lib_name=rapidjson

export folder=$src_dir/$lib_name
export repo_path=https://github.com/Tencent/$lib_name

source $scripts_root/../desktop/scripts/clonegit.sh

cd $folder
patch_file=$scripts_root/libs/rapidjson.patch
if ! patch -R -p0 -s -f --dry-run <$patch_file > /dev/null; then
  echo "rapidjson distro must be patched with $patch_file"
  patch -p0 <$patch_file
  echo "rapidjson patched"
fi

cd $lib_build_dir

export CXXFLAGS=-fPIC

cmake -G "Unix Makefiles" \
	    -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_TOOLCHAIN_FILE=$IOS_CMAKE \
            -DDEPLOYMENT_TARGET=$min_ios_version \
            -DARCHS=$arch \
            -DENABLE_BITCODE=$enable_bitcode \
            -DENABLE_VISIBILITY=$enable_visibility \
            -DENABLE_ARC=0 \
            -DPLATFORM=$ios_platform \
            -DCMAKE_INSTALL_PREFIX=$install_dir \
            -DRAPIDJSON_BUILD_EXAMPLES=Off \
            -DRAPIDJSON_BUILD_TESTS=Off \
            -DRAPIDJSON_BUILD_DOC=Off \
	        -DREGISTER_INSTALL_PREFIX=0 \
	    $folder

make -j$build_workers install
unset CXXFLAGS
