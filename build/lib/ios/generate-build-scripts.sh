#!/bin/bash

set -e

self=`basename $0`

if [ $# -eq 0 ]
	then
		echo "Usage: $self <lib>"
		exit -1
fi
export project=$1

export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
export working_dir=$PWD

if [ ! -f "$working_dir/ios-env.sh" ];
then
cp $scripts_root/ios-env.sh $working_dir/ios-env.sh
fi

source $scripts_root/cfg.sh

export targets=$working_dir/ios/scripts
if [ ! -d $targets ]; then
        mkdir -p $targets
fi

project_path=$targets/$project
if [ ! -d $project_path ]; then
	mkdir -p $project_path
fi

add_archs()
{
	build_type=$1
	bitcode_mode=$2
	visibility_mode=$3
	
	
	archs_rel_path=$project/$build_type/$bitcode_mode/$visibility_mode
	archs_scripts_dir=$targets/$archs_rel_path
	
	if [ ! -d $archs_scripts_dir ];
		then
			mkdir -p $archs_scripts_dir
	fi
	
	build_script=$archs_scripts_dir/1-build.sh
cat <<EOT > $build_script
#!/bin/bash

# *** This file is auto generated, do not edit! ***

set -e

export project=$project
export build_type=$build_type
export bitcode_mode=$bitcode_mode
export visibility_mode=$visibility_mode

export working_dir=$working_dir
current=\$PWD

cd $scripts_root/lib

./build-target.sh

cd \$current

EOT
	chmod +x $build_script
	
	all_archs=$archs_scripts_dir/0-all.sh
cat <<EOT > $all_archs
#!/bin/bash

# *** This file is auto generated, do not edit! ***

set -e

EOT
	chmod +x $all_archs

	for arch_ in $archs_list; do
		
        if [ "$arch_" = "simulator" ]
        then
        
            if [ "$bitcode_mode" = "bitcode" ] || [ "$visibility_mode" = "visible" ]
            then
                continue
            fi

            export arch_

            if [ "$host_arch" = "" ]
            then
                export arch=arm64
            else
                export arch=$host_arch
            fi
        else
        
            export arch=$arch_
        
        fi
        
		target_script=$archs_scripts_dir/$arch_.sh
		echo Adding $archs_rel_path/$arch_
		
		echo ./$arch_.sh >> $all_archs
		
cat <<EOT > $target_script
#!/bin/bash

# *** This file is auto generated, do not edit! ***

set -e

export arch=$arch
export arch_=$arch_
export arch_rel_path=$archs_rel_path/$arch_

echo "***********************"
echo Building \$arch_rel_path
echo "***********************"

$archs_scripts_dir/1-build.sh

echo "***********************"
echo Done \$arch_rel_path
echo "***********************"

EOT
	chmod +x $target_script
		
	done
}

process_visibilities()
{
	build_type=$1
	bitcode_mode=$2

	bitcode_root=$project_path/$build_type/$bitcode_mode
	if [ ! -d $bitcode_root ];
		then
			mkdir -p $bitcode_root
	fi	
	
	all_visibilities=$bitcode_root/all.sh
cat <<EOT > $all_visibilities
#!/bin/bash

# *** This file is auto generated, do not edit! ***

set -e

EOT
	chmod +x $all_visibilities
	
	if [ "$build_type" = "debug" ];
		then
			add_archs $build_type $bitcode_mode "normal"
			echo "cd normal && ./0-all.sh && cd .." >> $all_visibilities
		else
			for visibility_mode in $visibility_modes; do
				add_archs $build_type $bitcode_mode $visibility_mode
				echo "cd $visibility_mode && ./0-all.sh && cd .." >> $all_visibilities
			done				
	fi
}

for build_type in $build_types; do
		
	build_type_root=$project_path/$build_type
	if [ ! -d $build_type_root ];
		then
			mkdir -p $build_type_root
	fi
		
	all_bitcodes=$build_type_root/all.sh
cat <<EOT > $all_bitcodes
#!/bin/bash

# *** This file is auto generated, do not edit! ***

set -e

EOT
	chmod +x $all_bitcodes	
	
	if [ "$build_type" = "debug" ];
		then
			process_visibilities $build_type "native"
			echo "cd native && ./all.sh && cd .." >> $all_bitcodes			
		else
			for bitcode_mode in $bitcode_modes; do
				process_visibilities $build_type $bitcode_mode
				echo "cd $bitcode_mode && ./all.sh && cd .." >> $all_bitcodes
			done			
	fi
done
