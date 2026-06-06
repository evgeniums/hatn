export lib_name=sentry-native

export folder=$src_dir/$lib_name
export repo_path=https://github.com/getsentry/sentry-native
# Clone the pinned tag as a shallow copy; clonegit.sh passes $git_extra_args to git clone
export git_extra_args="--branch $sentry_version --depth 1"

source $scripts_root/scripts/clonegit.sh

cd $lib_build_dir

# Use crashpad backend for maximum reliability on desktop (ships a crashpad_handler binary).
# On Linux the curl transport requires libcurl-dev on the build host; on macOS the system
# libcurl is used automatically.  To opt out, add -DSENTRY_TRANSPORT=none here.
cmake -G "$cmake_gen_prefix Makefiles" \
        -DCMAKE_INSTALL_PREFIX=$root_dir \
        -DCMAKE_PREFIX_PATH=$root_dir \
        -DCMAKE_BUILD_TYPE=Release \
        -DSENTRY_BACKEND=crashpad \
        -DSENTRY_BUILD_SHARED_LIBS=OFF \
        -DSENTRY_BUILD_TESTS=OFF \
        -DSENTRY_BUILD_EXAMPLES=OFF \
        $folder

$make_tool install -j $build_workers install

unset git_extra_args
