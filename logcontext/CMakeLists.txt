PROJECT(hatnlogcontext)

SET (HEADERS
    include/hatn/logcontext/logcontext.h
    include/hatn/logcontext/record.h
    include/hatn/logcontext/context.h
    include/hatn/logcontext/logger.h
    include/hatn/logcontext/contextlogger.h
    include/hatn/logcontext/buflogger.h
    include/hatn/logcontext/streamlogger.h
    include/hatn/logcontext/loggerhandler.h
    include/hatn/logcontext/withlogger.h
    include/hatn/logcontext/loggererror.h
    include/hatn/logcontext/loggererrorcodes.h
    include/hatn/logcontext/filelogger.h
    include/hatn/logcontext/logconfigrecords.h
)

SET (HEADERS
    ${HEADERS}
)

SET (SOURCES
    src/record.cpp
    src/context.cpp
    src/logger.cpp
    src/contextlogger.cpp
    src/streamlogger.cpp
    src/loggererror.cpp
)

BUILD_HATN_MODULE()
