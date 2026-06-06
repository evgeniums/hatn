export lib_name=sentry-native

export folder=$src_dir/$lib_name
export repo_path=https://github.com/getsentry/sentry-native
# Clone the pinned tag as a shallow copy
export git_extra_args="--branch $sentry_version --depth 1"

source $scripts_root/../desktop/scripts/clonegit.sh

cd $lib_build_dir

# Use inproc backend: crashpad is not supported on iOS.
# SENTRY_TRANSPORT=none because the cross-compile environment has no libcurl;
# the application is expected to supply a custom transport or use the
# sentry_sdk wrapper which handles networking.
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
        -DSENTRY_BACKEND=inproc \
        -DSENTRY_TRANSPORT=none \
        -DSENTRY_BUILD_SHARED_LIBS=OFF \
        -DSENTRY_BUILD_TESTS=OFF \
        -DSENTRY_BUILD_EXAMPLES=OFF \
        $folder

make -j$build_workers install

unset git_extra_args
