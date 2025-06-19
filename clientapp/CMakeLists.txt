PROJECT(hatnclientapp)

SET (HEADERS
    include/hatn/clientapp/clientappdefs.h
    include/hatn/clientapp/clientapperror.h
    include/hatn/clientapp/clientapperrorcodes.h
    include/hatn/clientapp/clientbridge.h
    include/hatn/clientapp/clientapp.h
    include/hatn/clientapp/bridgeappcontext.h
    include/hatn/clientapp/notificationdispatcher.h
)

SET (SOURCES
    src/clientapperror.cpp
    src/clientbridge.cpp
    src/clientapp.cpp
    src/notificationdispatcher.cpp
)

IF(BUILD_ANDROID OR BUILD_IOS)

    SET (HEADERS
        ${HEADERS}
        include/hatn/clientapp/mobileapp.h
        include/hatn/clientapp/mobileplatformcontext.h        
        include/hatn/clientapp/testservicedb.h
    )

    SET (SOURCES
        ${SOURCES}
        src/mobileapp.cpp
        src/mobileplatformcontext.cpp
        src/testservicedb.cpp
    )

ENDIF()

BUILD_HATN_MODULE()
