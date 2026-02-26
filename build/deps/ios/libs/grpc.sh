export lib_name=grpc

export folder=$src_dir/$lib_name
export repo_path=https://github.com/$lib_name/$lib_name

export git_extra_args="-b v$grpc_version --depth 1 --shallow-submodules"
source $scripts_root/../desktop/scripts/clonegit.sh

#export lib_build_dir=$build_dir/$lib_name
#if [ -d "$lib_build_dir" ]; then
#    rm -rf $lib_build_dir
#fi
#mkdir -p $lib_build_dir

cd $lib_build_dir

C_FLAGS=(
  "-Wno-macro-redefined"
  "-Wno-error=incompatible-function-pointer-types"
  "-Wno-implicit-function-declaration"
  "-DUPB_USE_TSAN=0"
  "-DGRPC_ASAN_SUPPRESSED=1"
  "-DUPB_ASAN_GUARD_SIZE=0"
  "-DUPB_TSAN_PUBLISHED_MEMBER="
  # By defining these as (void), they become: (void)(args);
  # This is valid C and avoids the shell syntax error entirely.
  "-DUPB_TSAN_INIT_PUBLISHED=\(void\)"
  "-DUPB_TSAN_CHECK_PUBLISHED=\(void\)"
  "-DUPB_POISON_MEMORY_REGION=\(void\)"
  "-DUPB_UNPOISON_MEMORY_REGION=\(void\)"
)

cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=$IOS_CMAKE \
      -DDEPLOYMENT_TARGET=$min_ios_version \
      -DARCHS=$arch \
      -DENABLE_BITCODE=$enable_bitcode \
      -DENABLE_VISIBILITY=$enable_visibility \
      -DENABLE_ARC=0 \
      -DPLATFORM=$ios_platform \
      -DCMAKE_INSTALL_PREFIX=$install_dir \
      -DCMAKE_PREFIX_PATH=$install_dir \
      -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DgRPC_BUILD_CODEGEN=OFF \
      -DgRPC_BUILD_CSHARP_EXT=OFF \
      -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
      -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \
      -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF \
      -DgRPC_ZLIB_PROVIDER=package \
      -DgRPC_CARES_PROVIDER=package \
      -DgRPC_SSL_PROVIDER=package \
      -DCMAKE_C_COMPILER_WORKS=ON \
      -DCMAKE_CXX_COMPILER_WORKS=ON \
      -DCMAKE_C_FLAGS="${C_FLAGS[*]}" \
      -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
      -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
      -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
      -DCMAKE_IGNORE_PATH="/opt/homebrew" \
      -Dc-ares_DIR="$install_dir/lib/cmake/c-ares" \
      -DOPENSSL_INCLUDE_DIR="$install_dir/include" \
      -DOPENSSL_CRYPTO_LIBRARY="$install_dir/lib/libcrypto.a" \
      -DOPENSSL_SSL_LIBRARY="$install_dir/lib/libssl.a" \
      -DOPENSSL_USE_STATIC_LIBS=TRUE \
      -DCMAKE_CXX_FLAGS="-DSSL_get_peer_certificate=SSL_get1_peer_certificate" \
      $folder

cmake --build . -j$build_workers --target install

