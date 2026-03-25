#!/bin/bash

if [ -z "$dep_libs" ]; then
export dep_libs="openssl boost c-ares rapidjson lz4 rocksdb utf8proc"
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

