PROJECT(hatnadminclient)

SET (HEADERS
    include/hatn/adminclient/adminclient.h

    include/hatn/adminclient/userserviceconstants.h
)

SET (SOURCES
    src/adminclient.cpp
)

BUILD_HATN_MODULE()
