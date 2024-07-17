PROJECT(hatnrocksdb)

# include cmake file for rocksdb
INCLUDE(rocksdb/RocksDBTargets)

# define macro for rocksdb linking
MACRO(LINK_ROCKSDB target)
    TARGET_LINK_LIBRARIES(${target} PUBLIC rocksdb)

    IF(MSVC)
        TARGET_LINK_LIBRARIES(${target} PUBLIC lz4_static)
    ELSE()
        TARGET_LINK_LIBRARIES(${target} PUBLIC lz4)
    ENDIF()

    IF(WIN32)
        TARGET_LINK_LIBRARIES(${target} PUBLIC rpcrt4 shlwapi)
    ENDIF()
ENDMACRO(LINK_ROCKSDB)

# build library hatnrocksdbschema
SET(SCHEMA_HEADERS
    include/hatn/db/plugins/rocksdb/rocksdbschemadef.h
    include/hatn/db/plugins/rocksdb/rocksdberror.h
    include/hatn/db/plugins/rocksdb/rocksdbmodel.h
    include/hatn/db/plugins/rocksdb/rocksdbschema.h
    include/hatn/db/plugins/rocksdb/rocksdbhandler.h
    include/hatn/db/plugins/rocksdb/saveuniquekey.h
    include/hatn/db/plugins/rocksdb/ttlmark.h
)

SET(SCHEMA_DETAIL_HEADERS
    include/hatn/db/plugins/rocksdb/ipp/rocksdbschema.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp
    include/hatn/db/plugins/rocksdb/detail/ttlindexes.ipp
    include/hatn/db/plugins/rocksdb/detail/fieldtostringbuf.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbcreateobject.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbreadobject.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbfind.ipp
    include/hatn/db/plugins/rocksdb/detail/indexkeysearch.ipp
    include/hatn/db/plugins/rocksdb/detail/querypartitions.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbdelete.ipp
)

SET (SCHEMA_SOURCES
    src/rocksdbschema.cpp
    src/rocksdbhandler.cpp
    src/rocksdberror.cpp
    src/saveuniquekey.cpp
    src/ttlmark.cpp
)

ADD_LIBRARY(hatnrocksdbschema ${LINK_TYPE} ${SCHEMA_HEADERS} ${SCHEMA_SOURCES} ${SCHEMA_DETAIL_HEADERS})
IF(WIN32)
    # @TODO Fix PUBLIC
    TARGET_COMPILE_DEFINITIONS(hatnrocksdbschema PUBLIC -DBUILD_HATN_ROCKSDB_SCHEMA)
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(hatnrocksdbschema PUBLIC hatndb)
TARGET_INCLUDE_DIRECTORIES(hatnrocksdbschema PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
LINK_ROCKSDB(hatnrocksdbschema)

# build plugin hatnrocksdb
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
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC hatnrocksdbschema)
