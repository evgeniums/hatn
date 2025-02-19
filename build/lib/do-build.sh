#!/bin/bash

set -e

export android_scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )/android"
building_target=0

if [ ! -d "build" ];
then
    mkdir build
fi

_self_dir=$PWD
cd build

hatn_target_os=$hatn_platform
if [ "$hatn_platform" = "linux" ] || [ "$hatn_platform" = "macos" ]
then
    hatn_target_os="unix"
fi

if [ "$hatn_target_os" = "unix" ]
then

    echo "Build for $hatn_platform..."

    if [ ! -d "scripts" ];
    then
        mkdir scripts
    fi

      if [ ! -d "scripts/$hatn_lib" ];
      then
          if [[ "${hatn_path}" == "" ]];
          then
            export hatn_path="$PWD/../hatn"
          fi
          source ${hatn_path}/build/lib/${hatn_target_os}/generate-build-scripts.sh $hatn_lib
      fi

      if [[ "$hatn_build" == "release" ]];
      then
            if [[ "$hatn_link" == "static" ]];
            then
                script_name=release-${hatn_link}-dev.sh
            else
                script_name=release-${hatn_link}.sh
            fi
      else
          script_name=debug-${hatn_link}-dev.sh
      fi

      building_target=1
      source scripts/$hatn_lib/${hatn_compiler}/$script_name
fi

if [ "$hatn_target_os" = "android" ]
then
    echo "Build for $hatn_platform..."

    export working_dir=$PWD
    cd $android_scripts_root/lib
    building_target=1
    source ./build-arch.sh $hatn_lib $hatn_arch
fi

if [ "$hatn_target_os" = "ios" ]
then
    echo "Build for $hatn_platform..."

    if [ ! -d "ios/scripts/$hatn_lib" ];
    then
        source ${hatn_path}/build/lib/ios/generate-build-scripts.sh $hatn_lib
    fi

    export working_dir=$PWD

    building_target=1
    source ios/scripts/$hatn_lib/$hatn_build/$hatn_bitcode/$hatn_visibility/${hatn_arch}.sh
fi

if [ $building_target -eq 0 ]
then
    echo "Unknown target platform: $hatn_platform"
    echo "Please set hatn_platform to one of supported platforms: linux | macos | android | ios"
    exit 1
fi

cd $_self_dir
