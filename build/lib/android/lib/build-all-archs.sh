#!/bin/bash

set -e

project=$1

if [ -f "$working_dir/android-env.sh" ];
then
	source $working_dir/android-env.sh
fi

export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"/..
source $scripts_root/cfg.sh
source $scripts_root/paths.sh

build_all()
{	
	for tuple in $archs_list ; do
		IFS=","
		read -ra arr <<< "$tuple"
		arch=${arr[0]}
		toolchain=${arr[1]}
		IFS=" "

		$PWD/build-arch.sh $project $toolchain
	done
	echo Done
}

build_all
