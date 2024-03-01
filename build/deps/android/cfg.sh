#!/bin/bash

# list of supported archs and toolchains
if [ -z "$archs_list" ]; then
export archs_list="arm,arm-linux-androideabi arm,aarch64 x86,x86 x86,x86_64"
fi

if [ -z "$build_workers" ]; then
export build_workers=8
fi

if [ -z "$libs_path" ]; then
export libs_path=$scripts_root/libs
fi
