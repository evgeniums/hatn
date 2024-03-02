# change version and download link to whatever you want
version=$iconv_version
download_link=https://ftp.gnu.org/pub/gnu/libiconv

ICONV_SRC=$src_dir/libiconv-$version
archive_name=libiconv-$version.tar.gz

unpack_archive()
{
	echo "Unpacking $archive_name ..."
	cd $src_dir
	tar zxf $downloads_dir/$archive_name
	cd -
}

if [ ! -d $ICONV_SRC ];
then
	if [ -f $downloads_dir/$archive_name ];
		then
			unpack_archive
		else
			echo iconv sources not found, downloading...
			curl -L $download_link/$archive_name --output $downloads_dir/$archive_name
			unpack_archive
	fi
fi

keep_path=$PATH

build_path=$toolchain_build_path/iconv
if [ -d $build_path ]; 
    then 
		rm -rf $build_path 
fi

mkdir -p $build_path
cd $build_path


if [ "$toolchain" = "x86_64" ];
then

env api_level=$android_api_level \
NDK=$ANDROID_NDK_ROOT \
$scripts_root/libs/iconv-env-x86_64.sh \
$ICONV_SRC/configure \
--host=$toolchain \
--enable-static=yes  \
--prefix=$toolchain_install_path \
&& make -j6 install

else

export PATH=$toolchain_path/bin:$PATH

export CFLAGS=--sysroot=$toolchain_sysroot
export CXXFLAGS="--sysroot=$toolchain_sysroot --std=libc++"
export LDFLAGS=--sysroot=$toolchain_sysroot


$ICONV_SRC/configure --host=$toolchain --with-sysroot=$toolchain_sysroot --prefix=$toolchain_install_path --enable-static=yes  --enable-shared=no
make -j$build_workers install

fi

cd -

export PATH=$keep_path
unset CFLAGS
unset CXXFLAGS
unset LDFLAGS
unset ANDROID_HOST
unset ANDROID_ARCH
