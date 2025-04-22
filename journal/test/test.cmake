SET (TEST_SOURCES
    ${JOURNAL_TEST_SRC}/testjournal.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

SET(TEST_FILES
    ${JOURNAL_TEST_SRC}/assets/config.jsonc
)

ADD_HATN_CTESTS(journal ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestJournal)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
    COPY_LIBRARY_HERE(hatnnetwork${LIB_POSTFIX} ../network/)
    COPY_LIBRARY_HERE(hatncrypt${LIB_POSTFIX} ../crypt/)    
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
    COPY_LIBRARY_HERE(hatndb${LIB_POSTFIX} ../db/)
    COPY_LIBRARY_HERE(hatnapp${LIB_POSTFIX} ../app/)
    COPY_LIBRARY_HERE(hatnapi${LIB_POSTFIX} ../api/)
    COPY_LIBRARY_HERE(hatnjournal${LIB_POSTFIX} ../journal/)
    IF (HATN_PLUGIN_rocksdb)
        COPY_LIBRARY_HERE(hatnrocksdbschema${LIB_POSTFIX} ../db/plugins/rocksdb)
        COPY_LIBRARY(hatnrocksdb${LIB_POSTFIX} ../db/plugins/rocksdb plugins/db)
        IF (HATN_PLUGIN_openssl)
            MESSAGE(STATUS "Copying openssl plugin to test folder")
            COPY_LIBRARY(hatnopenssl${LIB_POSTFIX} ../crypt/plugins/openssl plugins/crypt)
        ENDIF()
    ENDIF()
ENDFUNCTION(TestJournal)

ADD_CUSTOM_TARGET(journaltest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
ADD_CUSTOM_TARGET(journaltest-files SOURCES ${TEST_FILES})

