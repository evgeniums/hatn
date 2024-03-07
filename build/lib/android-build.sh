#!/bin/bash

set -e

export hatn_lib=$1
export hatn_compiler=$2
export hatn_arch=$3
export hatn_build=$4
export hatn_link=$5
export hatn_plugins=$6

export hatn_platform=android

cd "$(dirname "$0")"
script_dir="$(pwd)"
cd -

source ${script_dir}/do-build.sh