PROJECT(hatnapi)

SET (HEADERS
    include/hatn/api/api.h
    include/hatn/api/apierror.h
    include/hatn/api/apierrorcodes.h
)

SET (HEADERS
    ${HEADERS}
)

SET (SOURCES
    src/apierror.cpp
)

BUILD_HATN_MODULE()
