PROJECT(hatnlogcontext)

SET (HEADERS
    include/hatn/logcontext/logcontext.h
    include/hatn/logcontext/record.h
    include/hatn/logcontext/context.h
    include/hatn/logcontext/logger.h
    include/hatn/logcontext/contextlogger.h
)

SET (HEADERS
    ${HEADERS}
)

SET (SOURCES
    src/context.cpp
    src/contextlogger.cpp
)

BUILD_HATN_MODULE()
