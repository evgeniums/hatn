#!/bin/bash

if [ -z "$dep_libs" ]; then
export dep_libs="iconv openssl c-ares rapidjson boost lz4 gflags rocksdb utf8proc"
# rocksdb"
fi

if [ -z "$openssl_version" ]; then
export openssl_version=3.2.1
fi

if [ -z "$boost_version" ]; then
export boost_version=1.84.0
fi

if [ -z "$grpc_version" ]; then
export grpc_version=1.78.1
fi

if [ -z "$iconv_version" ]; then
export iconv_version=1.16
fi
