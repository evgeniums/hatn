export lib_name=opus

export folder=$src_dir/$lib_name
export repo_path=https://github.com/xiph/$lib_name
# Pinned tag, shallow -- same arrangement the desktop sentry.sh uses.
export git_extra_args="-b v$opus_version --depth 1"

source $scripts_root/../desktop/scripts/clonegit.sh

cd $lib_build_dir

# WARNING: the webrtc call plugin already bundles its own third_party/opus. Linking this archive into
# the same iOS app is exactly the duplicate-symbol setup that produced the BoringSSL failure (see
# rtc/webrtc/deps/boringssl_prefix). Screen the resulting static lib with `nm` for leaked opus_*
# symbols before linking both, and decide between sharing webrtc's opus or a symbol-prefixed build.
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
	    -DOPUS_BUILD_SHARED_LIBRARY=OFF \
	    -DOPUS_BUILD_TESTING=OFF \
	    -DOPUS_BUILD_PROGRAMS=OFF \
	    $folder

make -j$build_workers install

unset git_extra_args
