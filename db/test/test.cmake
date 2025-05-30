SET (TEST_SOURCES
    ${DB_TEST_SRC}/testobject.cpp
    ${DB_TEST_SRC}/testinitplugins.cpp
    ${DB_TEST_SRC}/testopenclose.cpp
    ${DB_TEST_SRC}/testschema.cpp
    ${DB_TEST_SRC}/testrocksdbschema.cpp
    ${DB_TEST_SRC}/testrocksdbop.cpp
    ${DB_TEST_SRC}/testcrud.cpp

    ${DB_TEST_SRC}/testfind.cpp
    ${DB_TEST_SRC}/models1.cpp

    ${DB_TEST_SRC}/testfindplain.cpp
    ${DB_TEST_SRC}/modelplain.cpp

    ${DB_TEST_SRC}/testfindembedded.cpp
    ${DB_TEST_SRC}/modelembedded.cpp

    ${DB_TEST_SRC}/testfindcompound.cpp
    ${DB_TEST_SRC}/modelcompound.cpp

    ${DB_TEST_SRC}/testfindcompound2.cpp
    ${DB_TEST_SRC}/modelcompound2.cpp

    ${DB_TEST_SRC}/testupdate.cpp
    ${DB_TEST_SRC}/testupdatenested.cpp

    ${DB_TEST_SRC}/testrepeated.cpp
    ${DB_TEST_SRC}/modelsrep.cpp

    ${DB_TEST_SRC}/testpartitions.cpp
    ${DB_TEST_SRC}/testttl.cpp
    ${DB_TEST_SRC}/testtransaction.cpp
    ${DB_TEST_SRC}/testmodeltopics.cpp

    ${DB_TEST_SRC}/testupdateserialize.cpp
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
    ${DB_TEST_SRC}/modelplain.h    
    ${DB_TEST_SRC}/finddefs.h
    ${DB_TEST_SRC}/findplain.h
    ${DB_TEST_SRC}/findcases.ipp
    ${DB_TEST_SRC}/modelembedded.h
    ${DB_TEST_SRC}/findembedded.h
    ${DB_TEST_SRC}/findhandlers.ipp
    ${DB_TEST_SRC}/findqueries.ipp
    ${DB_TEST_SRC}/findcheckers.ipp
    ${DB_TEST_SRC}/modelcompound.h
    ${DB_TEST_SRC}/findcompound.h
    ${DB_TEST_SRC}/findcompoundqueries.ipp
    ${DB_TEST_SRC}/modelcompound2.h
    ${DB_TEST_SRC}/findcompound2.h
    ${DB_TEST_SRC}/findcompoundqueries2.ipp
    ${DB_TEST_SRC}/updatescalarops.ipp
    ${DB_TEST_SRC}/modelsrep.h
)

ADD_LIBRARY(${MODULE_TEST_LIB} STATIC ${HATN_TEST_THREAD_SOURCES} ${TEST_LIB_SOURCES} ${TEST_LIB_HEADERS})
TARGET_INCLUDE_DIRECTORIES(${MODULE_TEST_LIB} PRIVATE ${TEST_BINARY_DIR} ${PLUGIN_SRC_DIRS})
TARGET_COMPILE_DEFINITIONS(${MODULE_TEST_LIB} PRIVATE -DBUILD_TEST_DB)
IF (HATN_PLUGIN_openssl)
    ADD_DEPENDENCIES(${MODULE_TEST_LIB} hatnopenssl)
ENDIF()

IF (BUILD_STATIC)
    MESSAGE(STATUS "Linking ${MODULE_TEST_LIB} with crypt plugins for static build")
    LINK_HATN_PLUGINS(${MODULE_TEST_LIB} crypt)

    IF (HATN_PLUGIN_openssl)
        SET (OPENSSL_PLUGIN_SRC_DIR "${HATN_SOURCE_DIR}/crypt/plugins/openssl/include")
        MESSAGE(STATUS "Add include directory for static build for ${MODULE_TEST_LIB}: ${OPENSSL_PLUGIN_SRC_DIR}")
        TARGET_INCLUDE_DIRECTORIES(${MODULE_TEST_LIB} PRIVATE ${OPENSSL_PLUGIN_SRC_DIR})
    ENDIF()
ENDIF()

SET(HATN_TEST_THREAD_SOURCES "")

ADD_HATN_MODULES(${MODULE_TEST_LIB} PUBLIC db)

IF (HATN_PLUGIN_rocksdb)
    MESSAGE(STATUS "Linking db tests with hatnrocksdbschema")
    TARGET_LINK_LIBRARIES(${MODULE_TEST_LIB} PUBLIC hatnrocksdbschema)
ENDIF()

ADD_HATN_CTESTS(db ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestDb)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
    COPY_LIBRARY_HERE(hatndb${LIB_POSTFIX} ../db/)
    COPY_LIBRARY_HERE(hatncrypt${LIB_POSTFIX} ../crypt/)
    IF (HATN_PLUGIN_rocksdb)
        COPY_LIBRARY_HERE(hatnrocksdbschema${LIB_POSTFIX} ../db/plugins/rocksdb)
        IF (HATN_PLUGIN_openssl)
            MESSAGE(STATUS "Copying openssl plugin to test folder")
            COPY_LIBRARY(hatnopenssl${LIB_POSTFIX} ../crypt/plugins/openssl plugins/crypt)
        ENDIF()
    ENDIF()
ENDFUNCTION(TestDb)

IF (MINGW AND BUILD_DEBUG)
    # Fix string table overflow when compiling in debug mode
    SET_SOURCE_FILES_PROPERTIES(${TEST_SOURCES} PROPERTIES COMPILE_FLAGS -Os)
    SET_SOURCE_FILES_PROPERTIES(${TEST_LIB_SOURCES} PROPERTIES COMPILE_FLAGS -Os)
ENDIF ()

IF (MSVC)
    # Fix string table overflow when compiling in debug mode
    SET_SOURCE_FILES_PROPERTIES(${TEST_SOURCES} PROPERTIES COMPILE_FLAGS "/bigobj")
    SET_SOURCE_FILES_PROPERTIES(${TEST_LIB_SOURCES} PROPERTIES COMPILE_FLAGS "/bigobj")
ENDIF ()

ADD_CUSTOM_TARGET(dbtest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
