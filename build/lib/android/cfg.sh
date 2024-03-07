#!/bin/bash

set -e

if [ -z "$hatn_api_level" ];
then
export hatn_api_level=21
fi

export api_level=$hatn_api_level
export android_platform=android-$api_level

# list of supported archs and toolchains
if [ -z "$archs_list" ];
then
export archs_list="arm,arm-linux-androideabi arm,aarch64 x86,x86 x86,x86_64"
fi

if [ -z "$enable_translations" ];
then
	export enable_translations=1
fi

if [ -z "$build_workers" ];
then
        export build_workers=4
fi
