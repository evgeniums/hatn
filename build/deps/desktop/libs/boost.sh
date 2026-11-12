export underscore_version=$boost_version
export underscore_version=${underscore_version//./_}

export lib_name=boost_$underscore_version

export archive=$lib_name.7z
export download_link=https://archives.boost.io/release/$boost_version/source/$archive

export folder=$src_dir/$lib_name

source $scripts_root/scripts/downloadandunpack.sh

cd $folder

# Boost 1.87 Asio bug: config_get() checks errno==ERANGE after strtoll() without clearing errno first,
# so a stale ERANGE left by any earlier call (e.g. CoreAudio) makes io_context creation throw
# "config out of range". Fixed upstream in 1.88; no-op for versions without the file or already patched.
if [ -f boost/asio/impl/config.hpp ] && ! grep -q "errno = 0;" boost/asio/impl/config.hpp
then
    patch -p0 -N -l < $scripts_root/libs/boost_asio_config_errno.patch
fi

./bootstrap.sh $compiler
./b2 toolset=$compiler \
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
    --with-stacktrace \
    --layout=system \
    variant=release \
    address-model=$address_model \
    link=shared \
    install \
    --prefix=$root_dir \
    --build-dir=$lib_build_dir \
    -j$build_workers
