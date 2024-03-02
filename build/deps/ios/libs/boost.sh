#!/bin/bash

export underscore_version=$boost_version
export underscore_version=${underscore_version//./_}

export lib_name=boost_$underscore_version

export archive=$lib_name.7z
export download_link=https://boostorg.jfrog.io/artifactory/main/release/$boost_version/source/$archive
export folder=$src_dir/$lib_name

source $scripts_root/../desktop/scripts/downloadandunpack.sh

cd $folder

extra_cppflags="-DBOOST_AC_USE_PTHREADS -DBOOST_SP_USE_PTHREADS -stdlib=libc++ -std=c++11 -Wall -pedantic -Wno-unused-variable $visibility_flags $bitcode_flags"
extra_cflags="-DBOOST_AC_USE_PTHREADS -DBOOST_SP_USE_PTHREADS -Wall -pedantic -Wno-unused-variable $visibility_flags $bitcode_flags"

user_config=$lib_build_dir/user-config.jam
rm -f $user_config

cat <<EOF > $user_config	
using clang-darwin : $toolchain_arch
	: xcrun --sdk $target_sdk clang++
	: <cxxflags>"-miphoneos-version-min=$min_ios_version -arch $arch $target_endianity $extra_cppflags"
	  <cflags>"-miphoneos-version-min=$min_ios_version -arch $arch $target_endianity $extra_cflags"
	  <linkflags>"-arch $arch -liconv"
	  <striper>
	;
EOF

#echo user_config.jam:
#cat $user_config 

./bootstrap.sh

compile_boost="./b2 -j$build_workers \
    --with-date_time \
    --with-exception \
    --with-filesystem \
    --with-iostreams \
    --with-locale \
    --with-program_options \
    --with-random \
    --with-regex \
    --with-system \
    --with-test \
    --with-thread \
    --with-timer \
    --with-container \
    toolset=clang-darwin-$toolchain_arch \
    -sICONV_PATH=$sdk_install_prefix \
    variant=$build_type \
    --layout=system \
    target-os=iphone \
    link=static \
    runtime-link=static \
    --build-dir=$lib_build_dir \
    --prefix=$install_dir \
    --user-config=$user_config \
    install"

echo $compile_boost
$compile_boost
