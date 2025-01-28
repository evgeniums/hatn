#!/bin/bash

if [ -z "$min_ios_version" ]; then
export min_ios_version=15.1
fi

if [ -z "$bitcode_modes" ]; then
export bitcode_modes="native bitcode"
fi

if [ -z "$visibility_modes" ]; then
export visibility_modes="normal hidden"
fi

if [ -z "$archs_list" ]; then
export archs_list="arm64 simulator"
fi

if [ -z "$build_workers" ]; then
export build_workers=6
fi

if [ -z "$deps_root" ]; then
export deps_root=$scripts_root/libs
fi
