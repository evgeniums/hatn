#!/bin/bash

self=`basename $0`

set -eo pipefail

if [ $# -eq 0 ];
    then
		echo Usage: $self "<android_api_level>"
		exit 1
fi

export android_api_level=$1

echo "Building android-$android_api_level $platform"

set -e

if [ -z "$scripts_root" ]; then
export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
fi

if [ -z "$working_dir" ]; then
export working_dir=$PWD
fi

source $scripts_root/cfg.sh
source $scripts_root/libs-cfg.sh
source $scripts_root/paths.sh

export platform_root=$install_root/api-$android_api_level
if [ ! -d "$platform_root" ];
    then
        mkdir -p $platform_root
fi

export build_root=$platform_root/build
if [ ! -d "$build_root" ];
    then
        mkdir -p $build_root
fi

export install_root=$platform_root/root
if [ ! -d "$install_root" ];
	then
		mkdir -p $install_root
fi

if [ ! -d "$downloads_dir" ];
	then
		mkdir -p $downloads_dir
fi

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     export host_os=linux;;
    Darwin*)    export host_os=darwin;;
    *)          echo "Unsupported Host OS"; exit1;;
esac
echo "Building on ${host_os}"

# build dependencies for all supported archs

export platform=android-$android_api_level

build_arch()
{
    export arch=$1
    export toolchain=$2

    export toolchain_sysroot=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/${host_os}-x86_64/sysroot
    export toolchain_bin_path=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/${host_os}-x86_64/bin

    export toolchain_build_path=$build_root/$toolchain
    export toolchain_install_path=$install_root/$toolchain
    export build_dir=$toolchain_build_path

    export triple=arm-linux-androideabi
    if [ "$arch" = "x86" ];
    then
        export triple=i686-linux-android
    fi

    clang_prefix=$toolchain;
    binutil_prefix=$clang_prefix;

    if [ "$toolchain" = "x86" ];
    then
        clang_prefix=i686;
        binutil_prefix=$clang_prefix;
    fi
    export toolchain_clang=$toolchain_bin_path/$clang_prefix-linux-android$android_api_level-clang
    export toolchain_clangpp="$toolchain_bin_path/$clang_prefix-linux-android$android_api_level-clang++"
    export toolchain_ar=$toolchain_bin_path/llvm-ar
    export toolchain_ranlib=$toolchain_bin_path/llvm-ranlib
    export toolchain_ld=$toolchain_bin_path/ld
    export toolchain_strip=$toolchain_bin_path/llvm-strip
    export toolchain_nm=$toolchain_bin_path/llvm-nm

    export toolchain_name=$toolchain-linux-android-clang$toolchain_version

    if [ "$toolchain" = "arm-linux-androideabi" ];
    then
        export toolchain_clang=$toolchain_bin_path/armv7a-linux-androideabi$android_api_level-clang
        export toolchain_clangpp=$toolchain_bin_path/armv7a-linux-androideabi$android_api_level-clang++
        export  toolchain_name=$toolchain-clang$toolchain_version
    fi

    export CC=$toolchain_clang
    export CXX=$toolchain_clangpp
    export RANLIB=$toolchain_ranlib
    export AR=$toolchain_ar
    export LD=$toolchain_ld
    export AS=$CC
    export STRIP=$toolchain_strip
    export NM=$toolchain_nm
    
    if [[ $toolchain == *"64"* ]];
            then
                    export address_model=64
            else
                    export address_model=32
    fi

    echo Begin arch=$arch toolchain=$toolchain address_model=$address_model
    echo "oolchain_name=$toolchain_name"
    echo "android_api_level=$android_api_level"
    echo "clang=$toolchain_clang"
    echo "clang++=$toolchain_clangpp"
    echo "ar=$toolchain_ar"
    echo "ranlib=$toolchain_ranlib"
    echo "ld=$toolchain_ld"
    echo "as=$toolchain_as"
    echo "strip=$toolchain_strip"
    echo "nm=$toolchain_nm"

    echo Building dependencies...
    for lib in $dep_libs; do
        echo Building $lib...
        $deps_root/$lib.sh
    done

    unset CC
    unset AR
    unset RANLIB
    unset CXX
    unset LD
    unset AS
    unset STRIP
    unset NM

    echo "Done arch=$arch toolchain=$toolchain"
}

build_all()
{	
    for tuple in $archs_list ; do
        IFS=","
        read -ra arr <<< "$tuple"
        arch=${arr[0]}
        toolchain=${arr[1]}
        IFS=" "

        build_arch $arch $toolchain
    done
    echo Done android-$android_api_level platform
}

build_all

# @todo Make it configurable
# if [ -d "$build_root" ];
#     then
#         rm -rf $build_root
# fi
