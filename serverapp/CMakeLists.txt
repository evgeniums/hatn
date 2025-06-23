PROJECT(hatnserverapp)

SET (HEADERS
    include/hatn/serverapp/serverappdefs.h
    include/hatn/serverapp/localusercontroller.h
    include/hatn/serverapp/userdbmodels.h
    include/hatn/serverapp/userdbmodelsprovider.h

    include/hatn/serverapp/auth/authtokens.h
    include/hatn/serverapp/auth/authprotocol.h
    include/hatn/serverapp/auth/authprotocols.h
    include/hatn/serverapp/auth/sessioncontroller.h
    include/hatn/serverapp/auth/sharedsecretprotocol.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/serverapp/ipp/localusercontroller.ipp
    include/hatn/serverapp/ipp/sessioncontroller.ipp
    include/hatn/serverapp/ipp/sharedsecretprotocol.ipp
)

SET (SOURCES
    src/serverapp.cpp
    src/userdbmodelsprovider.cpp
)

BUILD_HATN_MODULE()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC args)

IF (HATN_PLUGIN_rocksdb)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC hatnrocksdbschema)
ENDIF()
