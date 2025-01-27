#!/bin/bash

export lib_name=lz4

export folder=$src_dir/$lib_name
export repo_path=https://github.com/lz4/$lib_name

source $scripts_root/../desktop/scripts/clonegit.sh

cd $lib_build_dir
echo "Build dir=$build_dir"

cmake -G "Unix Makefiles" \
        -DCMAKE_TOOLCHAIN_FILE=$IOS_CMAKE \
        -DDEPLOYMENT_TARGET=$min_ios_version \
        -DARCHS=$arch \
        -DENABLE_BITCODE=$enable_bitcode \
        -DENABLE_VISIBILITY=$enable_visibility \
        -DENABLE_ARC=0 \
        -DPLATFORM=$ios_platform \
        -DCMAKE_INSTALL_PREFIX=$install_dir \
        -DREGISTER_INSTALL_PREFIX=0 \
        -DLZ4_BUILD_CLI=Off \
        -DBUILD_SHARED_LIBS=Off \
        -DBUILD_STATIC_LIBS=On \
        -DLZ4_BUILD_CLI=Off \
        -DLZ4_BUILD_LEGACY_LZ4C=Off \
        $folder/build/cmake
make -j$build_workers install

cd -
