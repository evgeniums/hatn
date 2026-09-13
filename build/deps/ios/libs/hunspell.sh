export lib_name=hunspell

export folder=$src_dir/$lib_name
export repo_path=https://github.com/hunspell/hunspell
export git_extra_args="-b v$hunspell_version --depth 1"

source $scripts_root/../desktop/scripts/clonegit.sh

# Same CMake shim the desktop script copies in -- hunspell ships no CMake build of its own, see
# build/deps/desktop/libs/hunspell-CMakeLists.txt's own header comment.
cp $scripts_root/../desktop/libs/hunspell-CMakeLists.txt $folder/CMakeLists.txt

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
	    $folder

make -j$build_workers install
unset CXXFLAGS
