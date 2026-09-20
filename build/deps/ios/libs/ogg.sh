export lib_name=ogg

export folder=$src_dir/$lib_name
export repo_path=https://github.com/xiph/$lib_name
# Pinned tag, shallow -- same arrangement the desktop sentry.sh uses.
export git_extra_args="-b v$ogg_version --depth 1"

source $scripts_root/../desktop/scripts/clonegit.sh

cd $lib_build_dir

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
	    -DREGISTER_INSTALL_PREFIX=0 \
	    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	    -DBUILD_SHARED_LIBS=OFF \
	    -DBUILD_TESTING=OFF \
	    -DINSTALL_DOCS=OFF \
	    $folder

make -j$build_workers install

unset git_extra_args
