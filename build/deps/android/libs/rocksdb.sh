export lib_name=rocksdb

export folder=$src_dir/$lib_name

export repo_path=https://github.com/facebook/$lib_name

source $scripts_root/../desktop/scripts/clonegit.sh

cd $folder/..
patch_file=$scripts_root/libs/rocksdb-android.patch
if ! patch -R -p0 -s -f --dry-run <$patch_file > /dev/null; then
  echo "rocksdb distro must be patched with $patch_file"
  patch -p0 <$patch_file
  echo "rocksdb patched"
fi
cd - > /dev/null

cd $lib_build_dir

set LZ4_INCLUDE=$root_dir/include
set LZ4_LIB_RELEASE=$root_dir/lib/liblz4.a

export CXXFLAGS=-fPIC

toolchain_name=$toolchain-linux-android-clang
if [ "$toolchain" = "arm-linux-androideabi" ];
then
    toolchain_name=$toolchain-clang
else
	if [ "$toolchain" = "i686" ];
	then
		export CXXFLAGS="-fPIC -D__i386__"
	fi
fi

echo "Android platform is $platform, toolchain $toolchain_name, compiler $toolchain_clangpp"

cmake -G "Unix Makefiles" \
	    -DCMAKE_BUILD_TYPE=Release \
	    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
	    -DANDROID_TOOLCHAIN_NAME=$toolchain_name \
	    -DANDROID_PLATFORM=$platform \
	    -DCMAKE_INSTALL_PREFIX=$toolchain_install_path \
	    -DWITH_TESTS=0 \
	    -DFAIL_ON_WARNINGS=0 \
	    -DWITH_GFLAGS=0 \
	    -DWITH_LZ4=1 \
	    -Dlz4_INCLUDE_DIRS=$toolchain_install_path/include \
	    -Dlz4_LIBRARIES=$toolchain_install_path/lib/liblz4.a \
	    -DROCKSDB_BUILD_SHARED=0 \
	    -DDISABLE_MARCH_NATIVE=1 \
	    $folder

make -j$build_workers install
unset CXXFLAGS
