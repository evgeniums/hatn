#!/bin/bash

echo Building toolchain $toolchain_path

if [ ! -d "$toolchain_path" ]; 
    then
        echo "Building toolchain=$toolchain for $platform with arch=$arch in $toolchain_path..."
		$ANDROID_NDK_ROOT/build/tools/make-standalone-toolchain.sh \
		    --arch=$arch \
		    --api=$platform \
		    --install-dir="$toolchain_path" 
else
    echo "Toolchain $toolchain for $platform and with arch $arch in $toolchain_path already built"
fi
