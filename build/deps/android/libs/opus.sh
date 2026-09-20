export lib_name=opus

export folder=$src_dir/$lib_name
export repo_path=https://github.com/xiph/$lib_name
# Pinned tag, shallow -- same arrangement the desktop sentry.sh uses.
export git_extra_args="-b v$opus_version --depth 1"

export build_dir=$toolchain_build_path
source $scripts_root/../desktop/scripts/clonegit.sh

cd $lib_build_dir

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
	    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	    -DBUILD_SHARED_LIBS=OFF \
	    -DOPUS_BUILD_SHARED_LIBRARY=OFF \
	    -DOPUS_BUILD_TESTING=OFF \
	    -DOPUS_BUILD_PROGRAMS=OFF \
	    $folder

make -j$build_workers install

unset git_extra_args
