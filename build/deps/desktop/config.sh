if [ -z "$dep_libs" ]; then
export dep_libs="openssl boost c-ares lz4 gflags rapidjson rocksdb utf8proc sentry"
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

# task-spellcheck.md. Not in the default $dep_libs above -- opt-in, same as grpc: build it with
# `dep_libs="... hunspell" ./build-macos-clang64.sh` (or the equivalent for your platform).
if [ -z "$hunspell_version" ]; then
export hunspell_version=1.7.2
fi

if [ -z "$cares_version" ]; then
export cares_version=1.34.5
fi

if [ -z "$build_workers" ]; then
export build_workers=6
fi

if [ -z "$deps_root" ]; then
export deps_root=$scripts_root/libs
fi
