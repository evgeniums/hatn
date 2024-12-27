PROJECT(hatnmq)

SET (HEADERS
    include/hatn/mq/mqdef.h
    include/hatn/mq/mqconfig.h
    include/hatn/mq/mq.h
    include/hatn/mq/mqinmem.h
)

SET (HEADERS
    ${HEADERS}
)

SET (SOURCES
	src/mq.cpp
)

BUILD_HATN_MODULE()
