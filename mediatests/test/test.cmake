SET (TEST_SOURCES
    ${MEDIATESTS_TEST_SRC}/testvoicecryptfile.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
    ${HATN_MEDIA_SRC}/test/testmediautils.h
)

ADD_HATN_CTESTS(mediatests ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestMediatests)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
    COPY_LIBRARY_HERE(hatncrypt${LIB_POSTFIX} ../crypt/)
    COPY_LIBRARY_HERE(hatnmedia${LIB_POSTFIX} ../media/)

    # The test loads the crypto plugin itself from ./plugins/crypt: HATN_TEST_PLUGINS is filled from the
    # plugins of the module under test, and this module has none.
    IF (HATN_PLUGIN_openssl)
        MESSAGE(STATUS "Copying openssl plugin to test folder")
        COPY_LIBRARY(hatnopenssl${LIB_POSTFIX} ../crypt/plugins/openssl plugins/crypt)
    ELSE()
        MESSAGE(WARNING "The mediatests tests need the openssl plugin: configure with BUILD_PLUGINS=openssl")
    ENDIF()
ENDFUNCTION(TestMediatests)

ADD_CUSTOM_TARGET(mediatests-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
