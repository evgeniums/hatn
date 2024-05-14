PROJECT(hatnrocksdb)

INCLUDE_DIRECTORIES(
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

SET (HEADERS
    include/hatn/db/plugins/rocksdb/rocksdbplugin.h
    include/hatn/db/plugins/rocksdb/rocksdbclient.h
)

SET (SOURCES
    src/rocksdbplugin.cpp
)

BUILD_HATN_PLUGIN(db)

INCLUDE(rocksdb/RocksDBTargets)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC rocksdb)
