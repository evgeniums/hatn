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
    include/hatn/logcontext/fileloggertraits.h
    include/hatn/logcontext/logconfigrecords.h
    include/hatn/logcontext/postasync.h
    include/hatn/logcontext/makeasynccallback.h
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
    src/filelogger.cpp
)

BUILD_HATN_MODULE()
