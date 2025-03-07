PROJECT(hatnmq)

SET (HEADERS
    include/hatn/mq/mq.h
    include/hatn/mq/scheduler.h
    include/hatn/mq/backgroundworker.h
)

SET (HEADERS
    ${HEADERS}
)

SET (SOURCES
	src/mq.cpp
)

BUILD_HATN_MODULE()
