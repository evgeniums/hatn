export lib_name=hunspell

export folder=$src_dir/$lib_name
export repo_path=https://github.com/hunspell/hunspell
export git_extra_args="-b v$hunspell_version --depth 1"

source $scripts_root/scripts/clonegit.sh

# hunspell ships autotools (autogen.sh -> configure) plus checked-in MSVC projects and no CMake
# build of its own -- a small CMake shim is copied over the clone's own (nonexistent)
# CMakeLists.txt so desktop/windows/ios/android all build it the same way. Same in-tree-fixup
# arrangement rapidjson.sh uses for rapidjson.patch, just a whole file instead of a patch.
cp $scripts_root/libs/hunspell-CMakeLists.txt $folder/CMakeLists.txt

cd $lib_build_dir

if ! [[ "$platform" == "windows" ]]; then
    export CXXFLAGS=-fPIC
fi

cmake -G "$cmake_gen_prefix Makefiles" \
        -DCMAKE_SH="CMAKE_SH-NOTFOUND" \
        -DCMAKE_INSTALL_PREFIX=$root_dir \
        -DCMAKE_BUILD_TYPE=Release \
        $folder
$make_tool -j$build_workers install

unset CXXFLAGS
