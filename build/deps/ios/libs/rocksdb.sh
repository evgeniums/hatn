export lib_name=rocksdb

export folder=$src_dir/$lib_name
export repo_path=https://github.com/facebook/$lib_name

source $scripts_root/../desktop/scripts/clonegit.sh

export patch_file=$scripts_root/libs/rocksdb-ios.patch
cd $folder
if ! patch -R -p0 -s -f --dry-run <$patch_file > /dev/null; then
  echo "rocksdb distro must be patched with $patch_file"
  patch -p0 <$patch_file
  echo "rocksdb patched"
fi
cd - > /dev/null

set GFLAGS_INCLUDE=$root_dir/include
set GFLAGS_LIB_RELEASE=$root_dir/lib/libgflags.a
set LZ4_INCLUDE=$root_dir/include
set LZ4_LIB_RELEASE=$root_dir/lib/liblz4.a

cd $lib_build_dir

export CXXFLAGS=-fPIC

cmake -G "Unix Makefiles" \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_TOOLCHAIN_FILE=$IOS_CMAKE \
            -DDEPLOYMENT_TARGET=$min_ios_version \
            -DARCHS=$arch \
            -DENABLE_BITCODE=$enable_bitcode \
            -DENABLE_VISIBILITY=$enable_visibility \
            -DENABLE_ARC=0 \
            -DPLATFORM=$ios_platform \
            -DCMAKE_INSTALL_PREFIX=$install_dir \
            -DWITH_TESTS=0 \
            -DFAIL_ON_WARNINGS=0 \
            -DWITH_GFLAGS=0 \
            -DWITH_LZ4=1 \
            -DROCKSDB_BUILD_SHARED=0 \
            -Dlz4_INCLUDE_DIRS=$install_dir/include \
            -Dlz4_LIBRARIES=$install_dir/lib/liblz4.a \
	    -DWITH_PERF_CONTEXT=0 \
	    -DWITH_IOSTATS_CONTEXT=0 \
	    -DUSE_RTTI=1 \
            $folder

make -j$build_workers install
unset CXXFLAGS
