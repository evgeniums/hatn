SET (TEST_SOURCES
    ${DB_TEST_SRC}/testobject.cpp
    ${DB_TEST_SRC}/testinitplugins.cpp
    ${DB_TEST_SRC}/testopenclose.cpp
    ${DB_TEST_SRC}/testschema.cpp
    ${DB_TEST_SRC}/testrocksdbschema.cpp
    ${DB_TEST_SRC}/testrocksdbop.cpp
    ${DB_TEST_SRC}/testcrud.cpp
    ${DB_TEST_SRC}/testfind.cpp
    ${DB_TEST_SRC}/testfindeq.cpp
    # ${DB_TEST_SRC}/models1.cpp
    # ${DB_TEST_SRC}/models2.cpp
    # ${DB_TEST_SRC}/models3.cpp
    # ${DB_TEST_SRC}/models4.cpp
    # ${DB_TEST_SRC}/models5.cpp
    # ${DB_TEST_SRC}/models6.cpp
    # ${DB_TEST_SRC}/models7.cpp
    # ${DB_TEST_SRC}/models8.cpp
    ${DB_TEST_SRC}/models9.cpp
    # ${DB_TEST_SRC}/models10.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

SET(MODULE_TEST_LIB dbtestlib)

SET(TEST_LIB_SOURCES
    ${DB_TEST_SRC}/initdbplugins.cpp
    ${DB_TEST_SRC}/preparedb.cpp    
)

SET(TEST_LIB_HEADERS
    ${DB_TEST_SRC}/initdbplugins.h
    ${DB_TEST_SRC}/preparedb.h
    ${DB_TEST_SRC}/models1.h
    ${DB_TEST_SRC}/models9.h
    ${DB_TEST_SRC}/models10.h
    ${DB_TEST_SRC}/findhandlers.h
)

ADD_LIBRARY(${MODULE_TEST_LIB} STATIC ${HATN_TEST_THREAD_SOURCES} ${TEST_LIB_SOURCES} ${TEST_LIB_HEADERS})
TARGET_INCLUDE_DIRECTORIES(${MODULE_TEST_LIB} PRIVATE ${TEST_BINARY_DIR} ${PLUGIN_SRC_DIRS})
TARGET_COMPILE_DEFINITIONS(${MODULE_TEST_LIB} PRIVATE -DBUILD_TEST_DB)

SET(HATN_TEST_THREAD_SOURCES "")

ADD_HATN_MODULES(${MODULE_TEST_LIB} PUBLIC db)
TARGET_LINK_LIBRARIES(${MODULE_TEST_LIB} PUBLIC hatnrocksdbschema)

ADD_HATN_CTESTS(db ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestDb)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
    COPY_LIBRARY_HERE(hatndb${LIB_POSTFIX} ../db/)
    COPY_LIBRARY_HERE(hatnrocksdbschema${LIB_POSTFIX} ../db/plugins/rocksdb)
ENDFUNCTION(TestDb)

IF (MINGW AND BUILD_DEBUG)
    # Fix string table overflow when compiling in debug mode
    SET_SOURCE_FILES_PROPERTIES(${TEST_SOURCES} PROPERTIES COMPILE_FLAGS -Os)
    SET_SOURCE_FILES_PROPERTIES(${TEST_LIB_SOURCES} PROPERTIES COMPILE_FLAGS -Os)
ENDIF ()

IF (MSVC)
    # Fix string table overflow when compiling in debug mode
    SET_SOURCE_FILES_PROPERTIES(${TEST_SOURCES} PROPERTIES COMPILE_FLAGS /bigobj)
    SET_SOURCE_FILES_PROPERTIES(${TEST_LIB_SOURCES} PROPERTIES COMPILE_FLAGS /bigobj)
ENDIF ()

ADD_CUSTOM_TARGET(dbtest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
