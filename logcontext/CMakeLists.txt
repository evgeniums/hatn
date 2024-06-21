PROJECT(hatnlogcontext)

SET (HEADERS
    include/hatn/logcontext/logcontext.h
    include/hatn/logcontext/record.h
    include/hatn/logcontext/context.h
)

SET (HEADERS
    ${HEADERS}
)

SET (SOURCES
    src/context.cpp
)

BUILD_HATN_MODULE()
