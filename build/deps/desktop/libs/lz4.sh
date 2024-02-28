export lib_name=lz4

export folder=$src_dir/$lib_name
export repo_path=https://github.com/lz4/$lib_name

source $scripts_root/scripts/clonegit.sh

cd $folder

$make_tool clean
$make_tool prefix=$root_dir uninstall
$make_tool prefix=$root_dir -j$build_workers install
