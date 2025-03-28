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
ENDFUNCTION(TestMq)

ADD_CUSTOM_TARGET(mqtest-src SOURCES ${TEST_HEADERS} ${TEST_SOURCES} ${SOURCES})
