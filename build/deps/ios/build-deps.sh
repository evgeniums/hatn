#!/bin/bash

export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

_self_dir=$pwd

if [ ! -d "deps" ]
then
    mkdir deps
fi
cd deps

export working_dir=$PWD

source $scripts_root/paths.sh
source $scripts_root/cfg.sh
source $scripts_root/libs-cfg.sh

if [ ! -d $src_dir ];	
then
	mkdir -p $src_dir
fi

if [ ! -d $build_dir ];	
then
	mkdir -p $build_dir
fi

if [ ! -d $downloads_dir ];	
then
	mkdir -p $downloads_dir
fi

if [ ! -d $install_root ];
then
	mkdir -p $install_root
fi

build_archs()
{
	export build_type=$1
	export bitcode_mode=$2
	export visibility_mode=$3
		
	if [ "$bitcode_mode" = "bitcode" ];	
    then
        export bitcode_flags="-fembed-bitcode"
        export enable_bitcode=1
    else
        export bitcode_flags=
        export enable_bitcode=0
	fi

	if [ "$visibility_mode" = "hidden" ];	
    then
        export visibility_flags="-fvisibility=hidden -fvisibility-inlines-hidden"
        export enable_visibility=1
    else
        export visibility_flags=
        export enable_visibility=0
	fi
		
		
	for arch_ in $archs_list; do
		
        export arch_
  
        if [ "$arch_" = "simulator" ]
        then
        
            if [ "$bitcode_mode" = "bitcode" ] || [ "$visibility_mode" = "visible" ]
            then
                continue
            fi

            echo "Building for iOS simulator"
            export ios_platform=SIMULATOR64
            if [ "$host_arch" = "" ]
            then
                export arch=arm64
            else
                export arch=$host_arch
                export toolchain_arch=$host_arch
            fi
            
            export target_sdk=iphonesimulator
            export target_endianity=
            export sdk_root=`xcrun --sdk iphonesimulator --show-sdk-platform-path`
            export sdk_install_prefix=$sdk_root/Developer/SDKs/iPhoneSimulator.sdk/usr
            export ios_platform=SIMULATOR64
            
        else
        
            echo "Building for iOS"
            export arch=$arch_
            export toolchain_arch=$arch
            export target_sdk=iphoneos
            export target_endianity=-D_LITTLE_ENDIAN
            export sdk_root=`xcrun --sdk iphoneos --show-sdk-platform-path`
            export sdk_install_prefix=$sdk_root/Developer/SDKs/iPhoneOS.sdk/usr

            if [ "$arch" = "arm64" ];
            then
                export ios_platform=OS64
            else
                export ios_platform=OS
            fi
        fi
        
		if [[ $arch == "64e" ]];
			then
  				export address_model=64e
  			else
  				export address_model=64
		fi
    
		export install_dir=$install_root/$build_type/$bitcode_mode/$visibility_mode/$arch_
	
		for lib in $dep_libs  
		do
			current=$PWD
			keep_path=$PATH
			
			echo "********************************************************"
			echo "******** Building $lib for $arch_ with $arch ***********"
			echo "********************************************************"
			
			$deps_root/$lib.sh $arch
			
			cd $current
			export PATH=$keep_path
		done
	
	done
}

process_visibilities()
{
	build_type=$1
	bitcode_mode=$2
	
	if [ "$build_type" = "debug" ];
		then
			build_archs $build_type $bitcode_mode "normal"
		else
			for visibility_mode in $visibility_modes; do
				build_archs $build_type $bitcode_mode $visibility_mode
			done				
	fi
}

build_types="release"
for build_type in $build_types; do
	
	if [ "$build_type" = "debug" ];
		then
			process_visibilities $build_type "native"			
		else
			for bitcode_mode in $bitcode_modes; do
				process_visibilities $build_type $bitcode_mode
			done			
	fi
done

cd $self_dir
