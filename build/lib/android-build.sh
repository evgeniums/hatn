#!/bin/bash

set -e

export hatn_lib=$1
export hatn_arch=$2
export hatn_api_level=$3
export hatn_plugins=$4

export hatn_platform=android

cd "$(dirname "$0")"
script_dir="$(pwd)"
cd -

source ${script_dir}/do-build.sh