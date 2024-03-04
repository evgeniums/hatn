#!/bin/bash

if [ -z "$system_boost" ];
then
export BOOST_ROOT=$boost_root
fi

if [ -z "$system_openssl" ];
then
export OPENSSL_ROOT_DIR=$openssl_root
fi

cmake -G "Unix Makefiles" \
	-DCMAKE_BUILD_TYPE=$build_type \
	-DCMAKE_INSTALL_PREFIX=$install_path \
	-DINSTALL_DEV=$install_dev \
	-DBUILD_STATIC=$build_static \
	-DENABLE_TRANSLATIONS=$enable_translations \
	-DDEV_MODULE=$module \
        -DBUILD_PLUGINS="$hatn_plugins" \
	$project_path

if [ -z "$codechecker" ];
then
    echo "Run make"
    make -j$build_workers install
else
    echo "Run CodeChecker"
    export PATH=~/codechecker/build/CodeChecker/bin:$PATH
    source ~/codechecker/venv/bin/activate
    CodeChecker log -o $working_dir/codechecker-$module.log -b "make -j$build_workers install"
    tag=$RANDOM
    CodeChecker analyze $working_dir/codechecker-$module.log -j$build_workers -i $project_path/codechecker-skip.cfg --tidy-config $project_path/clang-tidy.cfg -o ./codechecker$tag
    CodeChecker store -n hatn-$module ./codechecker$tag --tag $tag
fi
