#!/bin/bash.sh

if [[ "${src_root}" == "" ]];
then
    export src_root=$scripts_root/../../..
fi
export src_dir=$src_root

echo "src_dir=${src_dir}"

export build_root=$working_dir/builds/ios
export install_root=$working_dir/install/ios

# dependencies root
if [[ "$deps_root" == "" ]];
then
    if [[ "$deps_universal_root" == "" ]];
    then
        export deps_root=$src_dir/../deps/root-ios
    else
        export deps_root=$deps_universal_root/root-ios
    fi
fi

if [ ! -z "$gettext_path" ]
then
    export PATH=$gettext_path:$PATH
fi

export PATH=$PATH:/usr/local/bin

export deps_arch=$deps_root/release/$bitcode_mode/$visibility_mode/$arch_

export build_path=$build_root/$arch_rel_path
export install_path=$install_root/$arch_rel_path

export IOS_CMAKE=$hatn_path/thirdparty/ios-cmake/ios.toolchain.cmake

