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

echo "deps_root=$deps_root"

echo Begin toolchain $toolchain

if [ -z "$ANDROID_SDK_ROOT" ];
then
    export ANDROID_SDK_ROOT=~/Library/Android/sdk
fi
export ANDROID_PLATFORM_TOOLS=$ANDROID_SDK_ROOT/platform-tools
echo "ANDROID_PLATFORM_TOOLS=$ANDROID_PLATFORM_TOOLS"

source $scripts_root/lib/run.sh

if [[ "$PREPARE_TESTS" == "1" ]];
then

echo "Preparing tests script $toolchain $working_dir/run-tests.sh ..."
if [ -f "$working_dir/run-tests.sh" ]
then
    rm $working_dir/run-tests.sh
fi
if [ -d "$working_dir/result-xml" ]
then
    rm -rf $working_dir/result-xml
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

result_xml_dir="/data/local/tmp/test/result-xml"

\$platform_tools/adb shell "su root chmod 777 /data/local/tmp"
cd $build_path
rm -rf test/CMakeFiles
mkdir -p test/result-xml
touch test/result-xml/keeper
\$platform_tools/adb shell "rm -rf /data/local/tmp/test"
\$platform_tools/adb push test /data/local/tmp/
cd $working_dir
ctest -L ALL --verbose --test-dir $build_path/test -C release
\$platform_tools/adb pull /data/local/tmp/test/result-xml result-xml
\$platform_tools/adb shell "rm -rf /data/local/tmp/test"

EOT

else

cat <<EOT > $working_dir/run-tests.sh
#!/bin/bash

# *** This file is auto generated, do not edit! ***

echo "Auto testing is disabled for $toolchain"

EOT

fi

chmod +x $working_dir/run-tests.sh

fi

echo Done toolchain $toolchain
