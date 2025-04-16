PROJECT(hatnadminclient)

SET (HEADERS
    include/hatn/adminclient/adminclient.h
    include/hatn/adminclient/admin.h
    include/hatn/adminclient/admincontroller.h
    include/hatn/adminclient/adminserviceconfig.h
)

SET (SOURCES
    src/adminclient.cpp
)

BUILD_HATN_MODULE()
