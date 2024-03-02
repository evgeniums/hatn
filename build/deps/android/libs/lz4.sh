export lib_name=lz4

export folder=$src_dir/$lib_name
export repo_path=https://github.com/lz4//$lib_name

source $scripts_root/../desktop/scripts/clonegit.sh

cd $lib_build_dir
echo "Build dir=$build_dir"

cmake -G "Unix Makefiles" \
	    -DCMAKE_BUILD_TYPE=Release \
	    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
	    -DANDROID_TOOLCHAIN_NAME=$toolchain_name \
	    -DANDROID_PLATFORM=$platform \
	    -DCMAKE_INSTALL_PREFIX=$toolchain_install_path \
	    -DLZ4_BUILD_CLI=Off \
	    -DBUILD_SHARED_LIBS=Off \
	    -DBUILD_STATIC_LIBS=On \
	    -DLZ4_BUILD_CLI=Off \
	    -DLZ4_BUILD_LEGACY_LZ4C=Off \
	    $folder/build/cmake
make VERBOSE=1 install

cd -