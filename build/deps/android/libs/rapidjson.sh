export lib_name=rapidjson

export folder=$src_dir/$lib_name
export repo_path=https://github.com/Tencent/$lib_name

export build_dir=$toolchain_build_path
source $scripts_root/../desktop/scripts/clonegit.sh

cd $folder/..
patch_file=$scripts_root/libs/rapidjson.patch
if ! patch -R -p0 -s -f --dry-run <$patch_file > /dev/null; then
  echo "rapidjson distro must be patched with $patch_file"
  patch -p0 <$patch_file
  echo "rapidjson patched"
fi

cd $lib_build_dir

export CXXFLAGS=-fPIC

toolchain_name=$toolchain-linux-android-clang$toolchain_version
if [ "$toolchain" = "arm-linux-androideabi" ];
then
    toolchain_name=$toolchain-clang$toolchain_version
fi

echo "Android platform is $platform, toolchain $toolchain_name"

cmake -G "Unix Makefiles" \
	    -DCMAKE_BUILD_TYPE=Release \
	    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
	    -DANDROID_TOOLCHAIN_NAME=$toolchain_name \
	    -DANDROID_PLATFORM=$platform \
	    -DCMAKE_INSTALL_PREFIX=$toolchain_install_path \
	    -DREGISTER_INSTALL_PREFIX=0 \
	    -DRAPIDJSON_BUILD_EXAMPLES=Off \
	    -DRAPIDJSON_BUILD_TESTS=Off \
	    -DRAPIDJSON_BUILD_DOC=Off \
	    $folder

make -j$build_workers install
unset CXXFLAGS
