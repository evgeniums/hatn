SET (TEST_SOURCES
    ${GRPCCLIENT_TEST_SRC}/testconnect.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

ADD_HATN_CTESTS(grpcclient ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestGrpcclient)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base)
    COPY_LIBRARY_HERE(hatnnetwork${LIB_POSTFIX} ../network)
    COPY_LIBRARY_HERE(hatncrypt${LIB_POSTFIX} ../crypt)
    COPY_LIBRARY_HERE(hatnapi${LIB_POSTFIX} ../api)
    COPY_LIBRARY_HERE(hatnapp${LIB_POSTFIX} ../app)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext)
    COPY_LIBRARY_HERE(hatngrpcclient${LIB_POSTFIX} ../grpcclient)

    IF (HATN_PLUGIN_rocksdb)
        MESSAGE(STATUS "Copying rocksdb plugin to test folder")
        COPY_LIBRARY(hatnrocksdb${LIB_POSTFIX} ../db/plugins/rocksdb plugins/db)
        COPY_LIBRARY(hatnrocksdbschema${LIB_POSTFIX} ../db/plugins/rocksdb plugins/db)
        IF (HATN_PLUGIN_openssl)
            MESSAGE(STATUS "Copying openssl plugin to test folder")
            COPY_LIBRARY(hatnopenssl${LIB_POSTFIX} ../crypt/plugins/openssl plugins/crypt)
        ENDIF()
    ENDIF()

ENDFUNCTION(TestGrpcclient)

ADD_CUSTOM_TARGET(grpcclienttest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})

SET (TEST_JSON
    ${GRPCCLIENT_TEST_SRC}/assets/echo.jsonc
)

ADD_CUSTOM_TARGET(grpcclienttest-json SOURCES ${TEST_JSON})
