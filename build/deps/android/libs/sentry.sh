export lib_name=sentry-native

export folder=$src_dir/$lib_name
export repo_path=https://github.com/getsentry/sentry-native
# Clone the pinned tag as a shallow copy
export git_extra_args="--branch $sentry_version --depth 1"

export build_dir=$toolchain_build_path
source $scripts_root/../desktop/scripts/clonegit.sh

cd $lib_build_dir

# Use inproc backend: crashpad is not supported on Android.
# SENTRY_TRANSPORT=none because the NDK sysroot has no libcurl;
# the application is expected to supply a custom transport or use the
# sentry_sdk wrapper which handles networking.
toolchain_name=$toolchain-linux-android-clang
if [ "$toolchain" = "arm-linux-androideabi" ]; then
    toolchain_name=$toolchain-clang
fi

echo "Android platform is $platform, toolchain $toolchain_name"

cmake -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
        -DANDROID_TOOLCHAIN_NAME=$toolchain_name \
        -DANDROID_PLATFORM=$platform \
        -DCMAKE_INSTALL_PREFIX=$toolchain_install_path \
        -DSENTRY_BACKEND=inproc \
        -DSENTRY_TRANSPORT=none \
        -DSENTRY_BUILD_SHARED_LIBS=OFF \
        -DSENTRY_BUILD_TESTS=OFF \
        -DSENTRY_BUILD_EXAMPLES=OFF \
        $folder

make -j$build_workers install

unset git_extra_args
