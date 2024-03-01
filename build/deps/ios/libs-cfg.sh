#!/bin/bash

if [ -z "$dep_libs" ]; then
export dep_libs="iconv openssl boost c-ares rapidjson lz4 rocksdb"
fi

if [ -z "$openssl_version" ]; then
export openssl_version=3.2.1
fi

if [ -z "$boost_version" ]; then
export boost_version=1.84.0
fi


