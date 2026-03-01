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
ENDFUNCTION(TestGrpcclient)

ADD_CUSTOM_TARGET(grpcclienttest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
