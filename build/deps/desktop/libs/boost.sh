export underscore_version=$boost_version
export underscore_version=${underscore_version//./_}

export lib_name=boost_$underscore_version

export archive=$lib_name.7z
export download_link=https://archives.boost.io/release/$boost_version/source/$archive

export folder=$src_dir/$lib_name

source $scripts_root/scripts/downloadandunpack.sh

cd $folder

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
    --layout=system \
    variant=release \
    address-model=$address_model \
    link=shared \
    install \
    --prefix=$root_dir \
    --build-dir=$lib_build_dir \
    -j$build_workers
