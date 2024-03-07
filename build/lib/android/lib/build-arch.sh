#!/bin/bash

set -e

export project=$1
export toolchain=$2

if [ -f "$working_dir/android-env.sh" ];
then
    source $working_dir/android-env.sh
fi

export scripts_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"/..
source $scripts_root/cfg.sh
source $scripts_root/paths.sh

echo Begin toolchain $toolchain

$scripts_root/lib/run.sh

if [[ "$PREPARE_TESTS" == "1" ]];
then

echo "Preparing tests script $toolchain $working_dir/run-tests.sh ..."
if [ -f "$working_dir/run-tests.sh" ]
then
    rm $working_dir/run-tests.sh
fi
if [ -f "$working_dir/test-out.xml" ]
then
    rm $working_dir/test-out.xml
fi

build_path=$build_root/api-$api_level/$project/$toolchain

if [[ "$toolchain" == "x86_64" ]];
then

if [ ! -z "$hatn_test_name" ];
then
    echo "Enable only test $hatn_test_name"
    run_tests="--run_test=$hatn_test_name"
else
    echo "Enable all tests for api-$api_level"
fi

cat <<EOT > $working_dir/run-tests.sh
#!/bin/bash

set -e

# *** This file is auto generated, do not edit! ***

echo "Auto testing in emulator for $toolchain platform"

if [ -z "\$ANDROID_SDK_ROOT" ];
then
	export ANDROID_SDK_ROOT=~/Library/Android/sdk
fi
platform_tools=\$ANDROID_SDK_ROOT/platform-tools

\$platform_tools/adb shell "su root chmod 777 /data/local/tmp"
cd $build_path
rm -rf test/CMakeFiles
\$platform_tools/adb shell "rm -rf /data/local/tmp/test"
\$platform_tools/adb push test /data/local/tmp/
cd $working_dir
\$platform_tools/adb shell "cd /data/local/tmp/test && ./hatnlibs-test --logger=XML,all,/data/local/tmp/test/test-out.xml --logger=HRF,test_suite --report_level=no --result_code=no $run_tests"
\$platform_tools/adb pull /data/local/tmp/test/test-out.xml
\$platform_tools/adb shell "rm -rf /data/local/tmp/test"

EOT

cat <<EOT > $working_dir/run-tests-manual.sh
#!/bin/bash

set -e

# *** This file is auto generated, do not edit! ***

echo "Auto testing in emulator for $toolchain platform"

if [ -z "\$ANDROID_SDK_ROOT" ];
then
	export ANDROID_SDK_ROOT=~/Library/Android/sdk
fi
platform_tools=\$ANDROID_SDK_ROOT/platform-tools

\$platform_tools/adb shell "su root chmod 777 /data/local/tmp"
cd $build_path
rm -rf test/CMakeFiles
\$platform_tools/adb shell "rm -rf /data/local/tmp/test"
\$platform_tools/adb push test /data/local/tmp/
cd $working_dir
\$platform_tools/adb shell "cd /data/local/tmp/test && ./hatnlibs-test --log_level=test_suite $run_tests"
\$platform_tools/adb shell "rm -rf /data/local/tmp/test"

EOT


else

cat <<EOT > $working_dir/run-tests.sh
#!/bin/bash

# *** This file is auto generated, do not edit! ***

echo "Auto testing is disabled for $toolchain"

EOT

cat <<EOT > $working_dir/run-tests-manual.sh
#!/bin/bash

# *** This file is auto generated, do not edit! ***

echo "Auto testing is disabled for $toolchain"

EOT

fi

chmod +x $working_dir/run-tests.sh
chmod +x $working_dir/run-tests-manual.sh

fi

echo Done toolchain $toolchain
