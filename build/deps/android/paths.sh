#!/bin/bash

# set ndk root
if [ -z ${ANDROID_NDK_ROOT+x} ];
    then    
	export ANDROID_NDK_ROOT=~/Android/ndk-root
fi
echo ANDROID_NDK_ROOT=$ANDROID_NDK_ROOT

# setup folders

export src_dir=$working_dir/src
export downloads_dir=$working_dir/downloads
export install_root=$working_dir/android
