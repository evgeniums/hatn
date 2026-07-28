export build_dir=$toolchain_build_path
export lib_name=openssl-$openssl_version

export archive=$lib_name.zip
export download_link=https://codeload.github.com/openssl/openssl/zip/refs/tags/$lib_name
export folder=$src_dir/openssl-$lib_name

source $scripts_root/../desktop/scripts/downloadandunpack.sh

if [[ "$toolchain" == "arm-linux-androideabi" ]]
then
        target="android-arm"
fi
if [[ "$toolchain" == "aarch64" ]]
then
        target="android-arm64"
fi
if [[ "$toolchain" == "x86_64" ]]
then
        target="android-x86_64"
fi
if [[ "$toolchain" == "x86" ]]
then
        target="android-x86"
fi

keep_path=$PATH

export ANDROID_NDK_HOME=$ANDROID_NDK_ROOT
export PATH=$toolchain_bin_path:$PATH
echo PATH=$PATH

export CFLAGS=-fPIC

cd $lib_build_dir

#$folder/Configure $target -D__ANDROID_API__=$android_api_level --prefix=$toolchain_install_path -static no-shared no-tests enable-engine
# No "-static" here: per OpenSSL INSTALL.md it implies no-dso/no-pic/no-shared AND no-threads.
# no-threads makes every CRYPTO lock a no-op and the "thread-local" ERR state a shared global,
# which corrupts the heap under concurrent EVP use (e.g. parallel HKDF from RocksDB background
# threads -> Scudo "race on chunk header" abort). no-shared alone already yields static libs;
# PIC comes from CFLAGS=-fPIC above. "threads" is the default but kept explicit as a guard.
# After rebuilding, verify: llvm-nm --defined-only -A libcrypto.a | grep CRYPTO_THREAD_lock_new
# must point at threads_pthread.o, not threads_none.o.
$folder/Configure $target --prefix=$toolchain_install_path threads no-shared no-tests enable-engine no-apps
make install_sw

export PATH=$keep_path
