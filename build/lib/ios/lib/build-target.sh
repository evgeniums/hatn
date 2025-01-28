#!/bin/bash

set -e

keep_path=$PATH

export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"/..

if [ -f "$working_dir/ios-env.sh" ];
then
source $working_dir/ios-env.sh
fi
source $scripts_root/cfg.sh
source $scripts_root/paths.sh

if [ "$bitcode_mode" = "bitcode" ];
	then
		export enable_bitcode=1
	else
		export enable_bitcode=0
fi

if [ "$visibility_mode" = "hidden" ];
	then
		export enable_visibility=FALSE		
	else
		export enable_visibility=TRUE		
fi

if [ "$arch_" = "simulator" ]
    then
        export ios_platform=SIMULATOR
    else
        if [ "$arch" = "arm64" ];
        then
            export ios_platform=OS64
        else
            export ios_platform=OS
        fi
fi

if [ -d $build_path ];
then
	rm -rf $build_path
fi
mkdir -p $build_path

current=$PWD
cd $build_path

$current/run.sh

if [[ "$PREPARE_TESTS" == "1" ]];
then

echo "Preparing tests script $arch $working_dir/run-tests.sh ..."
if [ -f "$working_dir/run-tests.sh" ]
then
    rm $working_dir/run-tests.sh
fi


if [ "$arch" = "x86_64" ] || [ "$ios_platform" = "SIMULATOR64" ]
then

if [ ! -z "$hatn_test_name" ];
then
    if [[ $hatn_test_name == *"/"* ]]; then
            echo "Will run test CASE $hatn_test_name"
            ctest_args="-L CASE -R $hatn_test_name"
    else
            echo "Will run test SUITE $hatn_test_name"
            ctest_args="-L SUITE -R $hatn_test_name"
    fi
else
    echo "Will run all tests"
    ctest_args="-L ALL"
fi

cat <<EOT > $working_dir/run-tests.sh
#!/bin/bash

set -e

# *** This file is auto generated, do not edit! ***

echo "Auto testing in simulator"

export PATH=\$PATH:/usr/local/bin

if [ -d "$working_dir/result-xml" ]
then
    rm -rf $working_dir/result-xml
fi
mkdir -p $working_dir/result-xml

ctest $ctest_args --verbose --test-dir $build_path/test -C release

EOT

else

cat <<EOT > $working_dir/run-tests.sh
#!/bin/bash

# *** This file is auto generated, do not edit! ***

echo "Auto testing is disabled for $arch_"

EOT

fi

chmod +x $working_dir/run-tests.sh

fi

cd $current
export PATH=$keep_path
