SET (TEST_SOURCES
    ${LOGCONTEXT_TEST_SRC}/testlogcontext.cpp
)

SET (TEST_HEADERS
    ${HEADERS}
)

ADD_HATN_CTESTS(logcontext ${TEST_SOURCES} ${TEST_HEADERS})

FUNCTION(TestLogcontext)
    COPY_LIBRARY_HERE(hatncommon${LIB_POSTFIX} ../common/)
    COPY_LIBRARY_HERE(hatnlogcontext${LIB_POSTFIX} ../logcontext/)
ENDFUNCTION(TestBase)

ADD_CUSTOM_TARGET(logcontexttest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
