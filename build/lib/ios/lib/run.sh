#!/bin/sh

set -e

ios_sdk_version_x10=$(echo "($ios_deployment_target * 10)/1" | bc)

cmake -G "Unix Makefiles"  \
    -DCMAKE_TOOLCHAIN_FILE=$IOS_CMAKE \
    -DBoost_INCLUDE_DIR=$deps_arch/include \
    -DBoost_LIBRARY_DIR_RELEASE=$deps_arch/lib \
    -Dlz4_LIBRARIES=$deps_arch/lib/liblz4.a \
    -Dlz4_INCLUDE_DIRS=$deps_arch/include \
    -DOPENSSL_INCLUDE_DIR=$deps_arch/include \
    -DOPENSSL_CRYPTO_LIBRARY=$deps_arch/lib \
    -DOPENSSL_SSL_LIBRARY=$deps_arch/lib \
    -DCMAKE_BUILD_TYPE=$build_type \
    -DDEPLOYMENT_TARGET=$ios_deployment_target \
    -DARCHS=$arch \
    -DBUILD_STATIC=1 \
    -DBOOST_STATIC=1 \
    -DDEV_MODULE=$project \
    -DENABLE_BITCODE=$enable_bitcode \
    -DENABLE_VISIBILITY=$enable_visibility \
    -DCMAKE_INSTALL_PREFIX=$install_path \
    -DENABLE_ARC=0 \
    -DPLATFORM=$ios_platform \
    -DINSTALL_DEV=1 \
    -DIOS_SDK_VERSION_X10=$ios_sdk_version_x10 \
    -DBUILD_PLUGINS="$hatn_plugins" \
    -DBUILD_IOS=1 \
    $src_dir
    
make -j$build_workers install
