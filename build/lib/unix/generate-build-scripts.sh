#!/bin/bash

set -e

self=`basename $0`

if [ $# -eq 0 ]
    then
	echo "Usage: $self <project>"
	exit -1
fi

if [ -f "$PWD/../../../hatn.src" ]; then
    echo "Do not run build scripts in source directory!"
    echo "Please, invoke $self in other working directory"
    exit 1
fi

export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
source $scripts_root/paths.sh

unamestr=`uname`
if [[ "$unamestr" == 'Linux' ]]; then
   platform=linux
elif [[ "$unamestr" == 'Darwin' ]]; then
   platform=macos
else
	echo "Unsupported platform"
	exit -1
fi

export target=$1
targets=scripts/$target
if [ ! -d $targets ]; then
	mkdir -p $targets	
fi

add_toolchain()
{
	toolchain=$1

	if [ ! -f "env-$toolchain.sh" ]; then
		cp $scripts_root/lib/platforms/$platform/$toolchain-env.sh $toolchain-env.sh
	fi

	# create folder
	target_path=$targets/$toolchain
	if [ ! -d $target_path ]; then
		mkdir -p $target_path
	fi

	# put build.sh to the folder
	build_script=$target_path/build.sh
cat <<EOT > $build_script
#!/bin/bash

# *** This file is auto generated, do not edit! ***

export platform=$platform
export toolchain=$toolchain

script_dir="\$( cd "\$( dirname "\${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
working_dir=\$script_dir/../../..
invoke_dir=\$PWD
cd $scripts_root/lib
./build-target.sh
cd \$invoke_dir
EOT
	chmod +x $build_script

	# create all.sh	
	echo "#!/bin/bash" > $target_path/all.sh
	echo "" >> $target_path/all.sh
	echo "# *** This file is auto generated, do not edit! ***" >> $target_path/all.sh
	chmod +x $target_path/all.sh
	echo "" >> $target_path/all.sh

	# add configurations
	for file in $scripts_root/lib/configurations/*; do
		configuration=`basename $file`
		configuration="${configuration%.*}"
		target_name=$toolchain-$target-$configuration
		echo "Adding $target_name"

		target_script=$target_path/$configuration.sh
cat <<EOT > $target_script
#!/bin/bash

# *** This file is auto generated, do not edit! ***

export working_dir=\$PWD/../../..
export initial_path=\$PATH

export module=$target
export configuration=$configuration

echo "***********************"
echo Building $target_name
echo "***********************"

$target_path/build.sh

export PATH=\$initial_path
EOT

		chmod +x $target_script
		echo "./$configuration.sh" >> $target_path/all.sh
	done
}

# create all.sh	
echo "#!/bin/bash" > $targets/all.sh
echo "" >> $targets/all.sh
echo "# *** This file is auto generated, do not edit! ***" >> $targets/all.sh
chmod +x $targets/all.sh
echo "" >> $targets/all.sh


if [ "$platform" = "macos" ];
	then
		add_toolchain clang
		echo "cd clang && ./all.sh && cd -" >> $targets/all.sh
	else
		add_toolchain gcc
		echo "cd gcc && ./all.sh && cd -" >> $targets/all.sh
		add_toolchain clang
		echo "cd clang && ./all.sh && cd -" >> $targets/all.sh
fi
