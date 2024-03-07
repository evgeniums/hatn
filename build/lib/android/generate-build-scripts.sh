#!/bin/bash

set -e

self=`basename $0`

if [ $# -eq 0 ]
	then
		echo "Usage: $self <lib>"
		exit -1
fi

export project=$1

export working_dir=$PWD
export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

source $scripts_root/cfg.sh

export targets_path=$working_dir/scripts/android
if [ ! -d "$targets_path" ]; then
	mkdir -p $targets_path	
fi

project_path=$targets_path/$project
if [ ! -d "$project_path" ]; then
	mkdir -p $project_path
fi

if [ ! -f "$working_dir/android-env.sh" ]; then
	cp $scripts_root/android-env.sh $working_dir/android-env.sh
fi

all_toolchcains_script=$project_path/0-all.sh
cat <<EOT > $all_toolchcains_script
#!/bin/bash

# *** This file is auto generated, do not edit! ***

set -e

current_pwd=\$PWD
export scripts_root=$scripts_root
export working_dir=$working_dir
cd \$scripts_root/lib
./build-all-archs.sh $project
cd \$current_pwd
EOT
	chmod +x $all_toolchcains_script

for tuple in $archs_list ; do
	IFS=","
	read -ra arr <<< "$tuple"
	arch=${arr[0]}
	toolchain=${arr[1]}
	IFS=" "

	echo Adding $toolchain
	
	toolchain_script=$project_path/$toolchain.sh

cat <<EOT > $toolchain_script
#!/bin/bash

# *** This file is auto generated, do not edit! ***

set -e

export working_dir=$working_dir
current_pwd=\$PWD
export scripts_root=$scripts_root
cd \$scripts_root/lib
./build-arch.sh $project $toolchain
cd \$current_pwd
EOT
	chmod +x $toolchain_script

done

