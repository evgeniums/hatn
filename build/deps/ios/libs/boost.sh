#!/bin/bash

export underscore_version=$boost_version
export underscore_version=${underscore_version//./_}

export lib_name=boost_$underscore_version

export archive=$lib_name.7z
export download_link=https://archives.boost.io/release/$boost_version/source/$archive
export folder=$src_dir/$lib_name

source $scripts_root/../desktop/scripts/downloadandunpack.sh

cd $folder

# Boost 1.87 Asio bug: stale errno makes io_context creation throw "config out of range" (fixed in 1.88).
# See desktop/libs/boost.sh. No-op for versions without the file or already patched.
if [ -f boost/asio/impl/config.hpp ] && ! grep -q "errno = 0;" boost/asio/impl/config.hpp
then
    patch -p0 -N -l < $scripts_root/../desktop/libs/boost_asio_config_errno.patch
fi

sdk_path=`xcrun --sdk $target_sdk --show-sdk-path`
echo "sdk_path=$sdk_path"

extra_cppflags="-DBOOST_AC_USE_PTHREADS -DBOOST_SP_USE_PTHREADS -stdlib=libc++ -std=c++11 -Wall -pedantic -Wno-unused-variable $visibility_flags $bitcode_flags -isysroot $sdk_path"
extra_cflags="-DBOOST_AC_USE_PTHREADS -DBOOST_SP_USE_PTHREADS -Wall -pedantic -Wno-unused-variable $visibility_flags $bitcode_flags -isysroot $sdk_path"

if [ "$arch_" = "simulator" ]
then
    arch_opts="-target $arch-apple-ios$min_ios_version-simulator -mios-simulator-version-min=$min_ios_version"
else
    arch_opts="-target $arch-apple-ios$min_ios_version -mios-version-min=$min_ios_version"
fi

user_config=$lib_build_dir/user-config.jam
rm -f $user_config

cat <<EOF > $user_config	
using clang-darwin : $arch_
	: xcrun --sdk $target_sdk clang++
	: <architecture>$arch
      <cxxflags>"-arch $arch $target_endianity $extra_cppflags $arch_opts"
	  <cflags>"-arch $arch $target_endianity $extra_cflags $arch_opts"
	  <linkflags>"-isysroot $sdk_path -arch $arch -liconv"
	  <striper>
	;
EOF

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
    toolset=clang-darwin-$arch_ \
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
