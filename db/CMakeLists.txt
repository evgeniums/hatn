PROJECT(hatndb)

SET (HEADERS
    include/hatn/db/db.h
    include/hatn/db/dberror.h
    include/hatn/db/dberrorcodes.h
    include/hatn/db/dbplugin.h
    include/hatn/db/object.h
    include/hatn/db/objectid.h
    include/hatn/db/topic.h
    include/hatn/db/client.h
    include/hatn/db/field.h
    include/hatn/db/index.h
    include/hatn/db/model.h
    include/hatn/db/schema.h
    include/hatn/db/modelregistry.h
    include/hatn/db/query.h
    include/hatn/db/indexquery.h
    include/hatn/db/timepointfilter.h
    include/hatn/db/update.h
    include/hatn/db/transaction.h
    include/hatn/db/find.h
    include/hatn/db/encryptionmanager.h
    include/hatn/db/updateserialization.h
    include/hatn/db/asyncclient.h
    include/hatn/db/modelswrapper.h
    include/hatn/db/modelsprovider.h
    include/hatn/db/asyncmodelcontroller.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/db/ipp/updateunit.ipp
    include/hatn/db/ipp/updateserialization.ipp
    include/hatn/db/ipp/modelsprovider.ipp
)

SET (SOURCES
    src/dberror.cpp
    src/dbplugin.cpp
    src/client.cpp
    src/schema.cpp
    src/object.cpp
    src/modelregistry.cpp
    src/query.cpp
    src/modelsprovider.cpp
)

BUILD_HATN_MODULE()
