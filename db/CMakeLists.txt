PROJECT(hatndb)

SET (HEADERS
    include/hatn/db/db.h
    include/hatn/db/dberror.h
    include/hatn/db/dberrorcodes.h
    include/hatn/db/dbplugin.h
    include/hatn/db/object.h
    include/hatn/db/objectid.h
    include/hatn/db/crud.h
    include/hatn/db/namespace.h
    include/hatn/db/client.h
    include/hatn/db/index.h
    include/hatn/db/model.h
    include/hatn/db/schema.h
    include/hatn/db/modelregistry.h
    include/hatn/db/query.h
    include/hatn/db/indexquery.h
    include/hatn/db/timepointfilter.h
    include/hatn/db/update.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/db/ipp/objectid.ipp
    include/hatn/db/ipp/updateunit.ipp
)

SET (SOURCES
    src/dberror.cpp
    src/dbplugin.cpp
    src/client.cpp
    src/schema.cpp
    src/object.cpp
    src/objectid.cpp
    src/modelregistry.cpp
    src/query.cpp
)

BUILD_HATN_MODULE()
