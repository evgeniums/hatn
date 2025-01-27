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

cd $lib_build_dir

#$folder/Configure $target -D__ANDROID_API__=$android_api_level --prefix=$toolchain_install_path -static no-shared no-tests enable-engine
$folder/Configure $target --prefix=$toolchain_install_path -static no-shared no-tests enable-engine no-apps
make install_sw

export PATH=$keep_path
