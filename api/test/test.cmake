SET (TEST_SOURCES
    ${API_TEST_SRC}/testplaintcpconnection.cpp
    ${API_TEST_SRC}/testclientserver.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

ADD_HATN_CTESTS(api ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestApi)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
    COPY_LIBRARY_HERE(hatnnetwork${LIB_POSTFIX} ../network/)
    COPY_LIBRARY_HERE(hatncrypt${LIB_POSTFIX} ../crypt/)
    COPY_LIBRARY_HERE(hatnapi${LIB_POSTFIX} ../api/)
    COPY_LIBRARY_HERE(hatnapp${LIB_POSTFIX} ../app/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
ENDFUNCTION(TestApi)

ADD_CUSTOM_TARGET(apitest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
