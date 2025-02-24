#!/bin/bash

set -e

build_path=$build_root/api-$api_level/$project/$toolchain
install_path=$install_root/api-$api_level/$project/$toolchain
deps=$deps_root/api-$api_level/root/$toolchain

project_src=$src_dir

if [ -d $build_path ]; 
	then 
		rm -rf $build_path 
fi
mkdir -p $build_path
cd $build_path

toolchain_name=$toolchain-linux-android-clang
if [ "$toolchain" = "arm-linux-androideabi" ];
then
    toolchain_name=$toolchain-clang
fi

echo "Android platform is $android_platform, toolchain $toolchain_name"

export deps_arch=$deps

cmake -G "Unix Makefiles" \
			    -DBUILD_ANDROID=1 \
			    -DBoost_INCLUDE_DIR=$deps/include \
			    -DBoost_LIBRARY_DIR_RELEASE=$deps/lib \
			    -DOPENSSL_INCLUDE_DIR=$deps/include \
			    -DOPENSSL_CRYPTO_LIBRARY=$deps/lib \
			    -DOPENSSL_SSL_LIBRARY=$deps/lib \
			    -Dlz4_LIBRARIES=$deps/lib/liblz4.a \
			    -Dlz4_INCLUDE_DIRS=$deps/include \
			    -DLIBICONV_ROOT=$deps \
			    -DBUILD_STATIC=1 \
			    -DBOOST_STATIC=1 \
			    -DBoost_NO_SYSTEM_PATHS=On \
			    -DBoost_USE_STATIC_LIBS=On \
			    -DCMAKE_BUILD_TYPE=Release \
			    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
                -DANDROID_TOOLCHAIN_NAME=$toolchain_name \
			    -DANDROID_PLATFORM=$android_platform \
			    -DENABLE_TRANSLATIONS=$enable_translations \
			    -DCMAKE_INSTALL_PREFIX=$install_path \
			    -DINSTALL_DEV=1 \
			    -DDEV_MODULE=$project \
                -DBUILD_PLUGINS="$hatn_plugins" \
			    $project_src
make -j$build_workers install
cd -
