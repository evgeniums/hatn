export lib_name=utf8proc

export folder=$src_dir/$lib_name
export repo_path=https://github.com/JuliaStrings/$lib_name

source $scripts_root/scripts/clonegit.sh

cd $lib_build_dir

cmake -DCMAKE_INSTALL_PREFIX=$root_dir \
        -DCMAKE_PREFIX_PATH=$root_dir \
        $folder

$make_tool install -j $build_workers install
