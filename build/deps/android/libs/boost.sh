export underscore_version=$boost_version
export underscore_version=${underscore_version//./_}

export lib_name=boost_$underscore_version

export archive=$lib_name.7z
export download_link=https://archives.boost.io/release/$boost_version/source/$archive
export folder=$src_dir/$lib_name

source $scripts_root/../desktop/scripts/downloadandunpack.sh

toolset=clang
jamplatform=$toolchain
if [ "$arch" = "arm" ];
then
    toolset=$toolset-arm$address_model
    jamplatform=arm$address_model
fi

keep_pwd=$PWD
keep_path=$PATH

set -eu

cd $folder

# Boost 1.87 Asio bug: stale errno makes io_context creation throw "config out of range" (fixed in 1.88).
# See desktop/libs/boost.sh. No-op for versions without the file or already patched.
if [ -f boost/asio/impl/config.hpp ] && ! grep -q "errno = 0;" boost/asio/impl/config.hpp
then
    patch -p0 -N -l < $scripts_root/../desktop/libs/boost_asio_config_errno.patch
fi

export PATH=$toolchain_bin_path:$PATH

user_config=tools/build/src/user-config.jam
rm -f $user_config
cat <<EOF > $user_config
import os ;

using clang : $jamplatform
:
"$toolchain_clangpp"
:
<archiver>$toolchain_ar
<ranlib>$toolchain_ranlib
<toolset>clang:<cxxflags>"-std=libc++ --sysroot=$toolchain_sysroot -D__ANDROID_API__=$android_api_level --isystem=$toolchain_sysroot/usr/include/$triple"
;
EOF

./bootstrap.sh $toolset

./b2 -j$build_workers \
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
    address-model=$address_model \
    toolset=$toolset \
    architecture=$arch \
    variant=release \
    --layout=system \
    target-os=android \
    threading=multi \
    threadapi=pthread \
    link=static \
    runtime-link=static \
    -sICONV_PATH=$toolchain_install_path \
    --build-dir=$lib_build_dir \
    --prefix=$toolchain_install_path \
    cxxflags=-fPIC \
    -d2 \
    install

cd $keep_pwd
export PATH=$keep_path
