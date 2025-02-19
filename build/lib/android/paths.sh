#!/bin/bash

set -e

if [[ "${src_root}" == "" ]];
then
    export src_root=$scripts_root/../../..
fi
export src_dir=$src_root

export build_root=$working_dir/builds/android
export install_root=$working_dir/install/android

# dependencies root
if [[ "$deps_root" == "" ]];
then
    if [[ "$deps_universal_root" == "" ]];
    then
        export deps_root=$src_dir/../deps/android
    else
        export deps_root=$deps_universal_root/android
    fi
fi


# android NDK
if [ -z "$ANDROID_NDK_ROOT" ];
then
	export ANDROID_NDK_ROOT=~/Android/ndk-root
fi


export PATH=$PATH:/usr/local/bin