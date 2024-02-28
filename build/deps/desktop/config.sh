if [ -z "$dep_libs" ]; then
export dep_libs="openssl boost c-ares lz4 gflags rapidjson rocksdb"
fi

if [ -z "$openssl_version" ]; then
export openssl_version=3.2.1
fi

if [ -z "$boost_version" ]; then
export boost_version=1.84.0
fi

if [ -z "$build_workers" ]; then
export build_workers=6
fi