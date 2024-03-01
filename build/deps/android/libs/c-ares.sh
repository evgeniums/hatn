export lib_name=c-ares

export folder=$src_dir/$lib_name
export repo_path=https://github.com/c-ares/$lib_name

export build_dir=$toolchain_build_path
source $scripts_root/../desktop/scripts/clonegit.sh

cd $lib_build_dir

export CXXFLAGS=-fPIC

echo "Android platform is $platform, toolchain $toolchain_name"

cmake -G "Unix Makefiles" \
	    -DCMAKE_BUILD_TYPE=Release \
	    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
	    -DANDROID_TOOLCHAIN_NAME=$toolchain_name \
	    -DANDROID_PLATFORM=$platform \
	    -DCMAKE_INSTALL_PREFIX=$toolchain_install_path \
	    -DREGISTER_INSTALL_PREFIX=0 \
	    -DCARES_STATIC=On \
	    -DCARES_SHARED=Off \
	    -DCARES_BUILD_TOOLS=Off \
	    $folder

make -j$build_workers install
unset CXXFLAGS
