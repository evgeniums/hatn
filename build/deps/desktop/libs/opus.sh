export lib_name=opus

export folder=$src_dir/$lib_name
export repo_path=https://github.com/xiph/$lib_name
# Pinned tag, shallow -- same arrangement sentry.sh uses. clonegit.sh passes $git_extra_args to git clone.
export git_extra_args="-b v$opus_version --depth 1"

source $scripts_root/scripts/clonegit.sh

cd $lib_build_dir

# Static, position-independent, no test/demo programs. Bare libopus does not depend on libogg (Ogg
# paging is done by hatn media itself over libogg), so build order relative to ogg does not matter.
# OpusConfig.cmake is installed (OPUS_INSTALL_CMAKE_CONFIG_MODULE defaults ON), so
# FIND_PACKAGE(Opus CONFIG) resolves the Opus::opus target straight off $root_dir.
cmake -G "$cmake_gen_prefix Makefiles" \
        -DCMAKE_INSTALL_PREFIX=$root_dir \
        -DCMAKE_PREFIX_PATH=$root_dir \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DOPUS_BUILD_SHARED_LIBRARY=OFF \
        -DOPUS_BUILD_TESTING=OFF \
        -DOPUS_BUILD_PROGRAMS=OFF \
        $folder

$make_tool -j$build_workers install

# runner.sh sources every lib script in one shell, so do not leak the pin into the next lib's clone.
unset git_extra_args
