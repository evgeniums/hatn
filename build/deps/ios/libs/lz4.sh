#!/bin/bash

export lib_name=lz4

export folder=$src_dir/$lib_name
export repo_path=https://github.com/lz4/$lib_name

source $scripts_root/../desktop/scripts/clonegit.sh

cd $folder

DEVELOPER=`xcode-select -print-path`

if [[ "${arch}" == "i386" || "${arch}" == "x86_64" ]]; then
    if [[ "${arch}" == "i386" ]]; then
        target=darwin-i386-cc
    else
        target=darwin64-x86_64-cc
    fi
    PLATFORM="iPhoneSimulator"
else
    if [[ "${addres_model}" == "64" ]];
        then
            target=ios64-cross
        else
            target=ios-cross
    fi
    PLATFORM="iPhoneOS"
fi

export CROSS_TOP=$DEVELOPER/Platforms/${PLATFORM}.platform/Developer
export CROSS_SDK=${PLATFORM}.sdk
export CFLAGS="-arch ${arch} -isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} ${bitcode_flags} ${visibility_flags} -mios-version-min=${min_ios_version}"
export CPPFLAGS=$CFLAGS

make clean
make prefix=$install_dir uninstall
make prefix=$install_dir BUILD_SHARED=no install

cd -

unset CFLAGS
unset CPPFLAGS

