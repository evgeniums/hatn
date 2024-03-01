#!/bin/bash

if [ -z "$dep_libs" ]; then
export dep_libs="iconv openssl c-ares rapidjson boost lz4 gflags rocksdb"
# rocksdb"
fi

if [ -z "$openssl_version" ]; then
export openssl_version=3.2.1
fi

if [ -z "$boost_version" ]; then
export boost_version=1.84.0
fi

if [ -z "$iconv_version" ]; then
export iconv_version=1.16
fi
