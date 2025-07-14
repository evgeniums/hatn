SET (TEST_SOURCES
    ${CLIENTSERVERTESTS_TEST_SRC}/testhssauth.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

ADD_HATN_CTESTS(clientservertests ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestClientServerTests)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
    COPY_LIBRARY_HERE(hatnnetwork${LIB_POSTFIX} ../network/)
    COPY_LIBRARY_HERE(hatncrypt${LIB_POSTFIX} ../crypt/)
    COPY_LIBRARY_HERE(hatnapi${LIB_POSTFIX} ../api/)
    COPY_LIBRARY_HERE(hatnapp${LIB_POSTFIX} ../app/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
    COPY_LIBRARY_HERE(hatnclientserver${LIB_POSTFIX} ../clientserver/)
    COPY_LIBRARY_HERE(hatnclientapp${LIB_POSTFIX} ../clientapp/)
    COPY_LIBRARY_HERE(hatnserverapp${LIB_POSTFIX} ../serverapp/)

    IF (HATN_PLUGIN_rocksdb)
        MESSAGE(STATUS "Copying rocksdb plugin to test folder")
        COPY_LIBRARY(hatnrocksdb${LIB_POSTFIX} ../db/plugins/rocksdb plugins/db)
        COPY_LIBRARY(hatnrocksdbschema${LIB_POSTFIX} ../db/plugins/rocksdb plugins/db)
        IF (HATN_PLUGIN_openssl)
            MESSAGE(STATUS "Copying openssl plugin to test folder")
            COPY_LIBRARY(hatnopenssl${LIB_POSTFIX} ../crypt/plugins/openssl plugins/crypt)
        ENDIF()
    ENDIF()

ENDFUNCTION(TestClientServerTests)

ADD_CUSTOM_TARGET(clientservertest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})

SET (TEST_JSON
    ${CLIENTSERVERTESTS_TEST_SRC}/assets/hssauthclient.jsonc
    ${CLIENTSERVERTESTS_TEST_SRC}/assets/hssauthserver.jsonc
    ${CLIENTSERVERTESTS_TEST_SRC}/assets/hssauthserver-sess-token-exp.jsonc
    ${CLIENTSERVERTESTS_TEST_SRC}/assets/hssauthserver-sess-exp.jsonc
)

ADD_CUSTOM_TARGET(clientservertest-json SOURCES ${TEST_JSON})
