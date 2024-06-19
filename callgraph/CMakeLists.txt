PROJECT(hatncallgraph)

SET (HEADERS
    include/hatn/callgraph/callgraph.h
    include/hatn/callgraph/stacklogrecord.h
    include/hatn/callgraph/stacklog.h
)

SET (HEADERS
    ${HEADERS}
)

SET (SOURCES
    src/stacklog.cpp
)

BUILD_HATN_MODULE()
