#!/bin/bash

export lib_name=lz4

export folder=$src_dir/$lib_name
export repo_path=https://github.com/lz4//$lib_name

export build_dir=$toolchain_build_path
source $scripts_root/../desktop/scripts/clonegit.sh

cd $folder

keep_path=$PATH
export PATH=$toolchain_path/bin:$PATH

export CFLAGS=--sysroot=$toolchain_sysroot
export CXXFLAGS="--sysroot=$toolchain_sysroot --std=libc++"
export LDFLAGS=--sysroot=$toolchain_sysroot

make clean
make prefix=$toolchain_install_path uninstall
make prefix=$toolchain_install_path install

cd -

export PATH=$keep_path
unset CFLAGS
unset CXXFLAGS
unset LDFLAGS
unset ANDROID_HOST
unset ANDROID_ARCH
