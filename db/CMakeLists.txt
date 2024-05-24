PROJECT(hatndb)

SET (HEADERS
    include/hatn/db/db.h
    include/hatn/db/dberror.h
    include/hatn/db/dberrorcodes.h
    include/hatn/db/dbplugin.h
    include/hatn/db/object.h
    include/hatn/db/crud.h
    include/hatn/db/namespace.h
    include/hatn/db/client.h
    include/hatn/db/schema.h
)

SET (SOURCES
    src/dberror.cpp
    src/dbplugin.cpp
    src/client.cpp
    src/schema.cpp
)

BUILD_HATN_MODULE()
