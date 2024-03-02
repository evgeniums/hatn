export lib_name=c-ares

export folder=$src_dir/$lib_name
export repo_path=https://github.com/c-ares/$lib_name

source $scripts_root/../desktop/scripts/clonegit.sh

cd $lib_build_dir

export CXXFLAGS=-fPIC

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
	        -DCARES_STATIC=On \
	        -DCARES_SHARED=Off \
	        -DCARES_BUILD_TOOLS=Off \
	    $folder

make -j$build_workers install
unset CXXFLAGS
