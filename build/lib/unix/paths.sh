#!/bin/bash

if [[ "${src_root}" == "" ]];
then
    export src_root=$scripts_root/../../..
fi

export build_root=$working_dir/builds
export install_root=$working_dir/install
	
		
