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
    include/hatn/db/plugins/rocksdb/rocksdboperror.h
    include/hatn/db/plugins/rocksdb/rocksdbmodel.h
    include/hatn/db/plugins/rocksdb/rocksdbmodels.h
    include/hatn/db/plugins/rocksdb/rocksdbmodelt.h
    include/hatn/db/plugins/rocksdb/rocksdbschema.h
    include/hatn/db/plugins/rocksdb/rocksdbhandler.h
    include/hatn/db/plugins/rocksdb/saveuniquekey.h
    include/hatn/db/plugins/rocksdb/ttlmark.h
    include/hatn/db/plugins/rocksdb/indexkeysearch.h
    include/hatn/db/plugins/rocksdb/rocksdbkeys.h
    include/hatn/db/plugins/rocksdb/modeltopics.h
)

SET(SCHEMA_DETAIL_HEADERS
    include/hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp
    include/hatn/db/plugins/rocksdb/detail/ttlindexes.ipp
    include/hatn/db/plugins/rocksdb/detail/fieldtostringbuf.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbcreateobject.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbreadobject.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbfind.ipp
    include/hatn/db/plugins/rocksdb/detail/querypartitions.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbdelete.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbdeletemany.ipp    
    include/hatn/db/plugins/rocksdb/detail/rocksdbupdate.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbupdatemany.ipp
    include/hatn/db/plugins/rocksdb/detail/objectpartition.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbtransaction.ipp
    include/hatn/db/plugins/rocksdb/detail/saveobject.ipp
    include/hatn/db/plugins/rocksdb/detail/findmany.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbcount.ipp
    include/hatn/db/plugins/rocksdb/ipp/rocksdbmodelt.ipp
    include/hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp
    include/hatn/db/plugins/rocksdb/detail/rocksdbfindcb.ipp
)

SET (SCHEMA_SOURCES
    src/rocksdbschema.cpp
    src/rocksdbmodels.cpp
    src/rocksdbhandler.cpp
    src/rocksdberror.cpp
    src/rocksdboperror.cpp
    src/rocksdbkeys.cpp
    src/saveuniquekey.cpp
    src/savesingleindex.cpp
    src/indexkeysearch.cpp
    src/ttlmark.cpp
    src/modeltopics.cpp
)

ADD_LIBRARY(hatnrocksdbschema ${LINK_TYPE} ${SCHEMA_HEADERS} ${SCHEMA_SOURCES} ${SCHEMA_DETAIL_HEADERS})
IF(WIN32)
    # @TODO Fix PUBLIC
    TARGET_COMPILE_DEFINITIONS(hatnrocksdbschema PUBLIC -DBUILD_HATN_ROCKSDB_SCHEMA)
ENDIF(WIN32)

IF (MINGW AND BUILD_DEBUG)
    # Fix string table overflow when compiling in debug mode
    SET_SOURCE_FILES_PROPERTIES(src/indexkeysearch.cpp PROPERTIES COMPILE_FLAGS -Os)
ENDIF ()

TARGET_LINK_LIBRARIES(hatnrocksdbschema PUBLIC hatndb)
TARGET_INCLUDE_DIRECTORIES(hatnrocksdbschema PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
LINK_ROCKSDB(hatnrocksdbschema)

# build plugin hatnrocksdb
SET (HEADERS
    include/hatn/db/plugins/rocksdb/rocksdbdriver.h
    include/hatn/db/plugins/rocksdb/rocksdbplugin.h
    include/hatn/db/plugins/rocksdb/rocksdbclient.h
    include/hatn/db/plugins/rocksdb/ttlcompactionfilter.h
)

SET (SOURCES
    src/rocksdbplugin.cpp
    src/rocksdbclient.cpp
    src/ttlcompactionfilter.cpp
)

BUILD_HATN_PLUGIN(db)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC hatnrocksdbschema)
