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

export deps_root=$scripts_root/libs

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
		
		
	for arch in $archs_list; do
		
		export arch
		
                if [ "$arch" = "x86_64" ] || [ "$arch" = "i386" ]
                        then
                                if [ "$arch" = "i386" ];
                                        then
                                                export ios_platform=SIMULATOR
                                        else
                                                export ios_platform=SIMULATOR64
                                fi
                        else
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
		
		if [ "$bitcode_mode" = "bitcode" ];
			then
				if [ "$arch" = "i386" ] || [ "$arch" = "x86_64" ]
					then
						continue
				fi
		fi
	
		if [ "$arch" = "x86_64" ] || [ "$arch" = "i386" ]
			then
				export target_sdk=iphonesimulator
				export target_endianity=
				export sdk_root=`xcrun --sdk iphonesimulator --show-sdk-platform-path`
				export sdk_install_prefix=$sdk_root/Developer/SDKs/iPhoneSimulator.sdk/usr
				if [ "$arch" = "i386" ];
					then
						export toolchain_arch=x86
					else
						export toolchain_arch=x86_64
				fi
			else
				export target_sdk=iphoneos
				export target_endianity=-D_LITTLE_ENDIAN
				export sdk_root=`xcrun --sdk iphoneos --show-sdk-platform-path`
				export sdk_install_prefix=$sdk_root/Developer/SDKs/iPhoneOS.sdk/usr
				if [ "$arch" = "arm64" ];
					then
						export toolchain_arch=arm64
					else
						export toolchain_arch=arm
				fi						
		fi
	
		export build_arch_dir=$build_dir/$build_type/$bitcode_mode/$visibility_mode/$arch
		export install_dir=$install_root/$build_type/$bitcode_mode/$visibility_mode/$arch		
	
		for lib in $dep_libs  
		do
			current=$PWD
			keep_path=$PATH
			
			echo "******************************************"
			echo "******************************************"
			echo "******************************************"
			echo "********Building $lib for $arch***********"
			echo "******************************************"
			echo "******************************************"
			echo "******************************************"
			
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
