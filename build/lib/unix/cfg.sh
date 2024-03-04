#!/bin/bash

if [[ "$deps_root" == "" ]];
then
    if [[ "$deps_universal_root" == "" ]];
    then
        export deps_root=$src_root/../deps/root-${toolchain}
    else
        export deps_root=$deps_universal_root/root-${toolchain}
    fi
fi

if [ -z "$boost_root" ]
then
    export boost_root=$deps_root
fi

if [ -z "$openssl_root" ]
then
    export openssl_root=$deps_root
fi


if [ -z "$build_workers" ]
then
    export build_workers=6
fi

if [ -z "$enable_translations" ]
then
    export enable_translations=1
fi
	
