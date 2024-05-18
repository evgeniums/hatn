PROJECT(hatnrocksdb)

INCLUDE_DIRECTORIES(
    ${INCLUDE_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

SET (HEADERS
    include/hatn/db/plugins/rocksdb/rocksdbdriver.h
    include/hatn/db/plugins/rocksdb/rocksdbplugin.h
    include/hatn/db/plugins/rocksdb/rocksdbclient.h
)

SET (SOURCES
    src/rocksdbplugin.cpp
    src/rocksdbclient.cpp
)

BUILD_HATN_PLUGIN(db)

INCLUDE(rocksdb/RocksDBTargets)

# @todo PUBLIC only for static build

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC rocksdb)

IF(MSVC)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC liblz4_static)
ELSE()
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC lz4)
ENDIF()

IF(WIN32)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC rpcrt4 shlwapi)
ENDIF()
