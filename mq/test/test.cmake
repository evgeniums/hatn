SET (TEST_SOURCES
    ${MQ_TEST_SRC}/testbackgroundworker.cpp
    ${MQ_TEST_SRC}/testproducer.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

ADD_HATN_CTESTS(mq ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestMq)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnmq${LIB_POSTFIX} ../mq/)
    IF (HATN_PLUGIN_rocksdb)
        COPY_LIBRARY_HERE(hatnrocksdbschema${LIB_POSTFIX} ../db/plugins/rocksdb)
        COPY_LIBRARY(hatnrocksdb${LIB_POSTFIX} ../db/plugins/rocksdb plugins/db)
        IF (HATN_PLUGIN_openssl)
            MESSAGE(STATUS "Copying openssl plugin to test folder")
            COPY_LIBRARY(hatnopenssl${LIB_POSTFIX} ../crypt/plugins/openssl plugins/crypt)
        ENDIF()
    ENDIF()
ENDFUNCTION(TestMq)

SET (TEST_JSON
    ${MQ_TEST_SRC}/assets/client.jsonc
)

ADD_CUSTOM_TARGET(mqtest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES} ${TEST_JSON})

