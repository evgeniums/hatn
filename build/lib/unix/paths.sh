#!/bin/bash

if [[ "${src_root}" == "" ]];
then
    export src_root=$scripts_root/../../..
fi

if  [[ "${project_working_dir}" == "" ]];
then
    export $project_working_dir=$PWD/build
fi

export build_root=$project_working_dir/builds
export install_root=$project_working_dir/install
