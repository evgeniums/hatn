SET (TEST_SOURCES
    ${SERVERAPP_TEST_SRC}/testapp.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

ADD_HATN_CTESTS(serverapp ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestServerApp)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
    COPY_LIBRARY_HERE(hatnnetwork${LIB_POSTFIX} ../network/)
    COPY_LIBRARY_HERE(hatncrypt${LIB_POSTFIX} ../crypt/)
    COPY_LIBRARY_HERE(hatnapi${LIB_POSTFIX} ../api/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
    COPY_LIBRARY_HERE(hatndb${LIB_POSTFIX} ../db/)
    COPY_LIBRARY_HERE(hatnapp${LIB_POSTFIX} ../app/)
ENDFUNCTION(TestApp)

ADD_CUSTOM_TARGET(serverapptest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
