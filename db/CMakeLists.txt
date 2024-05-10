PROJECT(hatndb)

SET (HEADERS
    include/hatn/db/db.h
    include/hatn/db/dberror.h
    include/hatn/db/dberrorcodes.h
)

SET (HEADERS
    ${HEADERS}
)

SET (SOURCES
    src/dberror.cpp
)

BUILD_HATN_MODULE()
