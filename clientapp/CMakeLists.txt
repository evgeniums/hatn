PROJECT(hatnclientapp)

SET (HEADERS
    include/hatn/clientapp/clientappdefs.h
    include/hatn/clientapp/clientapperror.h
    include/hatn/clientapp/clientapperrorcodes.h
    include/hatn/clientapp/clientbridge.h
    include/hatn/clientapp/clientapp.h
    include/hatn/clientapp/bridgeappcontext.h
    include/hatn/clientapp/eventdispatcher.h
    include/hatn/clientapp/bridgemethod.h

    include/hatn/clientapp/systemservice.h
    include/hatn/clientapp/methodnetworkstatus.h
    include/hatn/clientapp/methodsyncdatetime.h
    include/hatn/clientapp/methodupdatesystemca.h

    include/hatn/clientapp/networkevent.h
    include/hatn/clientapp/systemcaevent.h

    include/hatn/clientapp/defaulttopic.h
    include/hatn/clientapp/clientappdbmodels.h
    include/hatn/clientapp/clientappdbmodelsprovider.h

    include/hatn/clientapp/clientappsettings.h
    include/hatn/clientapp/lockingcontroller.h
    include/hatn/clientapp/confirmationdescriptor.h

    include/hatn/clientapp/testservicedb.h
)

LIST (APPEND HEADERS
    include/hatn/clientapp/ipp/clientbridge.ipp
)

SET (SOURCES
    src/clientapperror.cpp
    src/clientbridge.cpp
    src/clientapp.cpp
    src/contextapp.cpp
    src/eventdispatcher.cpp
    src/systemservice.cpp
    src/methodnetworkstatus.cpp
    src/methodsyncdatetime.cpp
    src/methodupdatesystemca.cpp
    src/clientappdbmodelsprovider.cpp
    src/clientappsettings.cpp
    src/lockingcontroller.cpp

    src/testservicedb.cpp
)

# IF(BUILD_ANDROID OR BUILD_IOS)

    SET (HEADERS
        ${HEADERS}
        include/hatn/clientapp/mobileapp.h
        include/hatn/clientapp/mobileplatformcontext.h
    )

    SET (SOURCES
        ${SOURCES}
        src/mobileapp.cpp
        src/mobileplatformcontext.cpp        
    )

# ENDIF()

BUILD_HATN_MODULE()

IF (HATN_PLUGIN_rocksdb)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC hatnrocksdbschema)
ENDIF()
