
IF NOT DEFINED DEP_LIBS (
SET DEP_LIBS=openssl boost c-ares lz4 gflags rapidjson rocksdb
)
ECHO "Building libraries: %DEP_LIBS%"

IF NOT DEFINED OPENSSL_VERSION (
SET OPENSSL_VERSION=3.2.1
)
ECHO "Using OpenSSL version %OPENSSL_VERSION%"

IF NOT DEFINED BOOST_VERSION (
SET BOOST_VERSION=1.84.0
)
ECHO "Using Boost version %BOOST_VERSION%"

IF NOT DEFINED COMPILER_VERSION (
SET COMPILER_VERSION=14.3
)
FOR /f "tokens=1,2 delims=." %%a IN ("%COMPILER_VERSION%") do SET MSVC_COMPILER=v%%a%%b
ECHO "Using %COMPILER_VERSION% (%MSVC_COMPILER%) toolset"

SET BUILD_WORKERS=8