export lib_name=grpc

export folder=$src_dir/$lib_name
export repo_path=https://github.com/$lib_name/$lib_name

export git_extra_args="-b v$grpc_version --depth 1 --shallow-submodules"
source $scripts_root/scripts/clonegit.sh

cd $lib_build_dir

cmake -DCMAKE_INSTALL_PREFIX=$root_dir \
        -DCMAKE_PREFIX_PATH=$root_dir \
        -DgRPC_INSTALL=ON \
        -DgRPC_BUILD_TESTS=OFF \
        -DgRPC_BUILD_CSHARP_EXT=OFF \
        -DCMAKE_CXX_STANDARD=17 \
        -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
        -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \
        -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF \
        -DgRPC_CARES_PROVIDER=package \
        -DgRPC_SSL_PROVIDER=package \
        -Dc-ares_DIR="$root_dir/lib/cmake/c-ares" \
        -DOPENSSL_INCLUDE_DIR="$root_dir/include" \
        -DOPENSSL_CRYPTO_LIBRARY="$root_dir/lib/libcrypto.a" \
        -DOPENSSL_SSL_LIBRARY="$root_dir/lib/libssl.a" \
        $folder

$make_tool install -j $build_workers install
