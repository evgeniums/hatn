INCLUDE(hatn/Build)

# prepare sha1
COPY_PREBUILD_THIRDPARTY_HEADERS("${HATN_COMMON_SRC}/thirdparty/sha1")
COPY_PREBUILD_THIRDPARTY_HEADERS("${HATN_COMMON_SRC}/thirdparty/crc32")
COPY_PREBUILD_THIRDPARTY_HEADERS("${HATN_COMMON_SRC}/thirdparty/base64")
COPY_PREBUILD_THIRDPARTY_HEADERS("${HATN_COMMON_SRC}/thirdparty/fameta")
COPY_PREBUILD_THIRDPARTY_HEADERS("${HATN_COMMON_SRC}/thirdparty/shortallocator")
