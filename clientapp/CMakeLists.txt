PROJECT(hatnclientapp)

SET (HEADERS
    include/hatn/clientapp/clientappdefs.h
    include/hatn/clientapp/clientapperror.h
    include/hatn/clientapp/clientapperrorcodes.h
    include/hatn/clientapp/clientbridge.h
    include/hatn/clientapp/clientapp.h
    include/hatn/clientapp/mobileapp.h
    include/hatn/clientapp/mobileplatformcontext.h
    include/hatn/clientapp/bridgeappcontext.h
    include/hatn/clientapp/testservicedb.h
)

SET (SOURCES
    src/clientapperror.cpp
    src/clientbridge.cpp
    src/clientapp.cpp
    src/mobileapp.cpp
    src/mobileplatformcontext.cpp
    src/bridgeappcontext.cpp
    src/testservicedb.cpp
)

BUILD_HATN_MODULE()
