#!/bin/bash

export lib_name=openssl-apple

export folder=$src_dir/$lib_name
export repo_path=https://github.com/passepartoutvpn/$lib_name

source $scripts_root/../desktop/scripts/clonegit.sh

cd $lib_build_dir

export CFLAGS="${bitcode_flags} ${visibility_flags}"
export CONFIG_OPTIONS="--prefix=$install_dir no-asm no-shared no-dso no-hw enable-engine no-dynamic-engine"

if [[ "${arch}" == "x86_64" ]]; then
    target=ios-sim-cross-x86_64
else
    if [[ "$ios_platform" == "SIMULATOR64"  ]]
    then
        target=ios-sim-cross-arm64
    else
        if [[ "${addres_model}" == "64e" ]];
            then
                target=ios64-cross-arm64e
            else
                target=ios64-cross-arm64
        fi
    fi
fi

cd $lib_build_dir
$folder/build-libssl.sh --targets="$target" --version=$openssl_version

unset CONFIG_OPTIONS
unset CFLAGS
