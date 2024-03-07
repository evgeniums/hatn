#!/bin/bash

# IOS deployment target
if [ -z "$ios_deployment_target" ]
then
export ios_deployment_target=11.0
fi

# build types
if [ -z "$build_types" ]; then
export build_types="release debug"
fi

if [ -z "$bitcode_modes" ]; then
export bitcode_modes="native bitcode"
fi

if [ -z "$visibility_modes" ]; then
export visibility_modes="normal hidden"
fi

if [ -z "$archs_list" ]; then
export archs_list="arm64 x86_64"
fi

if [ -z "$build_workers" ]; then
export build_workers=6
fi
