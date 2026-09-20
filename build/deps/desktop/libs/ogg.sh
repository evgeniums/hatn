export lib_name=ogg

export folder=$src_dir/$lib_name
export repo_path=https://github.com/xiph/$lib_name
# Pinned tag, shallow -- same arrangement sentry.sh uses. clonegit.sh passes $git_extra_args to git clone.
export git_extra_args="-b v$ogg_version --depth 1"

source $scripts_root/scripts/clonegit.sh

cd $lib_build_dir

# Static, position-independent: hatnmedia is usually a shared lib and links this PRIVATE, so the
# archive has to be PIC. OggConfig.cmake is installed (INSTALL_CMAKE_PACKAGE_MODULE defaults ON), so
# FIND_PACKAGE(Ogg CONFIG) resolves the Ogg::ogg target straight off $root_dir.
cmake -G "$cmake_gen_prefix Makefiles" \
        -DCMAKE_INSTALL_PREFIX=$root_dir \
        -DCMAKE_PREFIX_PATH=$root_dir \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DINSTALL_DOCS=OFF \
        $folder

$make_tool -j$build_workers install

# runner.sh sources every lib script in one shell, so do not leak the pin into the next lib's clone.
unset git_extra_args
