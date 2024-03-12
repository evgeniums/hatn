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

build_path=$build_root/api-$api_level/$project/$toolchain

if [[ "$toolchain" == "x86_64" ]];
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

echo "Auto testing in emulator for $toolchain platform"

if [ -d "$working_dir/result-xml" ]
then
    rm -rf $working_dir/result-xml
fi

if [ -z "\$ANDROID_SDK_ROOT" ];
then
    export ANDROID_SDK_ROOT=~/Library/Android/sdk
fi
platform_tools=\$ANDROID_SDK_ROOT/platform-tools

\$platform_tools/adb shell "su root chmod 777 /data/local/tmp"
cd $build_path
rm -rf test/CMakeFiles
mkdir -p test/result-xml
touch test/result-xml/keeper
\$platform_tools/adb shell "rm -rf /data/local/tmp/test"
\$platform_tools/adb push test /data/local/tmp/
cd $working_dir
ctest $ctest_args --verbose --test-dir $build_path/test -C release
\$platform_tools/adb pull /data/local/tmp/test/result-xml
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
