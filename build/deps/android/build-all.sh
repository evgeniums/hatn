#!/bin/bash

set -e

if [ -z "$scripts_root" ]; then
export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
fi

if [ -z "$working_dir" ]; then

if [ ! -d "deps" ]; then
mkdir deps
fi

if [ ! -d "deps/src" ]; then
mkdir -p deps/src
fi

_current_dir=$PWD
cd deps

export working_dir=$PWD

cd $current_dir
fi

source $scripts_root/api-levels.sh

build_platform()
{
    for api_level in $platforms; do
        $scripts_root/build-platform-all.sh $api_level
    done
}

build_platform
