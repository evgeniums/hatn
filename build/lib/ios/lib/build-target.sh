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

if [ "$arch" = "x86_64" ] || [ "$arch" = "i386" ]
	then
		if [ "$arch" = "i386" ];
			then
				export ios_platform=SIMULATOR
			else
				export ios_platform=SIMULATOR64
		fi
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
if [ -f "$working_dir/test-out.xml" ]
then
    rm $working_dir/test-out.xml
fi

if [[ "$arch" == "x86_64" ]];
then

if [ ! -z "$hatn_test_name" ];
then
    echo "Enable only test $hatn_test_name"
    run_tests="--run_test=$hatn_test_name"
else
    echo "Enable all tests"
fi

if [ -z "$hatn_test_ios_device" ];
then
    echo "Run tests on booted device(s)"
    export hatn_test_ios_device=booted
else
    echo "Run tests on device $hatn_test_ios_device"
fi

cat <<EOT > $working_dir/run-tests.sh
#!/bin/bash

set -e

# *** This file is auto generated, do not edit! ***

echo "Auto testing in simulator for x86_64 platform"
xcrun simctl spawn $hatn_test_ios_device $build_path/test/hatnlibs-test.app/hatnlibs-test --logger=XML,all,$working_dir/test-out.xml --logger=HRF,test_suite --report_level=no --result_code=no $run_tests

EOT

cat <<EOT > $working_dir/run-tests-manual.sh
#!/bin/bash

set -e

# *** This file is auto generated, do not edit! ***

echo "Auto testing in simulator for x86_64 platform"
xcrun simctl spawn $hatn_test_ios_device $build_path/test/hatnlibs-test.app/hatnlibs-test --log_level=test_suite

EOT

chmod +x $working_dir/run-tests-manual.sh

else

cat <<EOT > $working_dir/run-tests.sh
#!/bin/bash

# *** This file is auto generated, do not edit! ***

echo "Auto testing is disabled for $arch"

EOT

fi

chmod +x $working_dir/run-tests.sh

fi

cd $current
export PATH=$keep_path
