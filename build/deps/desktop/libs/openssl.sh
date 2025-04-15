#!/bin/bash

export lib_name=openssl-$openssl_version

export archive=$lib_name.zip
export download_link=https://codeload.github.com/openssl/openssl/zip/refs/tags/$lib_name
export folder=$src_dir/openssl-$lib_name

source $scripts_root/scripts/downloadandunpack.sh

export keep_toolchain=$toolchain

echo "Toolchain $toolchain"

if [[ $compiler == "gcc" ]]
then
    echo "Adding -fPIC to CFLAGS"
    export CFLAGS=-fPIC
fi

if [[ $toolchain == "clang" ]] 
then
    if [ $platform = "linux" ]
     then
	if [ $address_model -eq 64 ] 
	then
	    if [[ "$arch" == "arm64" ]]
	    then
		export toolchain="linux-aarch64"
	    else
		export toolchain="linux-$arch-clang"
	    fi
	else
	    export toolchain=linux-x86-clang 
	fi
    else
        export toolchain="darwin64-$arch-cc"
    fi 
fi

echo "Building OpenSSL for $platform $toolchain $address_model"

cd $lib_build_dir
$folder/Configure no-asm --prefix=$root_dir --openssldir=$root_dir/var/lib/openssl $toolchain
make install

export toolchain=$keep_toolchain
