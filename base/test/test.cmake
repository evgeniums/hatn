SET (TEST_SOURCES
    ${BASE_TEST_SRC}/testconfigtree.cpp
    ${BASE_TEST_SRC}/testconfigtreevalue.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

ADD_HATN_CTESTS(base ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestBase)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatndataunit${LIB_POSTFIX} ../dataunit/)
    COPY_LIBRARY_HERE(hatnbase${LIB_POSTFIX} ../base/)
ENDFUNCTION(TestBase)

ADD_CUSTOM_TARGET(basetest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
