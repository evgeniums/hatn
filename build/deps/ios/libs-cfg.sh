#!/bin/bash

if [ -z "$dep_libs" ]; then
export dep_libs="openssl boost c-ares rapidjson lz4 gflags rocksdb"
fi

if [ -z "$openssl_version" ]; then
export openssl_version=3.2.1
fi

if [ -z "$boost_version" ]; then
export boost_version=1.84.0
fi


