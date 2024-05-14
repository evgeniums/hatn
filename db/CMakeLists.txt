PROJECT(hatndb)

SET (HEADERS
    include/hatn/db/db.h
    include/hatn/db/dberror.h
    include/hatn/db/dberrorcodes.h
    include/hatn/db/dbplugin.h
    include/hatn/db/object.h
    include/hatn/db/crud.h
    include/hatn/db/namespace.h
)

SET (HEADERS
    ${HEADERS}
)

SET (SOURCES
    src/dberror.cpp
    src/dbplugin.cpp
)

BUILD_HATN_MODULE()
