#!/bin/bash

if [ -z "$dep_libs" ]; then
export dep_libs="iconv openssl c-ares rapidjson boost lz4 gflags rocksdb utf8proc sentry"
fi

if [ -z "$sentry_version" ]; then
export sentry_version=0.14.2
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

if [ -z "$cares_version" ]; then
export cares_version=1.34.5
fi

# task-spellcheck.md. Not in the default $dep_libs above -- opt-in, same as grpc.
if [ -z "$hunspell_version" ]; then
export hunspell_version=1.7.2
fi

# hatn media module (Ogg/Opus). Not in the default $dep_libs above -- opt-in, same as grpc and
# hunspell.
if [ -z "$ogg_version" ]; then
export ogg_version=1.3.5
fi

if [ -z "$opus_version" ]; then
export opus_version=1.5.2
fi
