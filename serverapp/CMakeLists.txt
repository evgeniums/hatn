PROJECT(hatnserverapp)

SET (HEADERS
    include/hatn/serverapp/serverappdefs.h
    include/hatn/serverapp/localusercontroller.h
    include/hatn/serverapp/userdbmodels.h
    include/hatn/serverapp/userdbmodelsprovider.h

    include/hatn/serverapp/auth/authtokens.h
    include/hatn/serverapp/auth/authprotocol.h
    include/hatn/serverapp/auth/authprotocols.h    
    include/hatn/serverapp/auth/sharedsecretprotocol.h

    include/hatn/serverapp/encryptedtoken.h

    include/hatn/serverapp/sessiondbmodels.h
    include/hatn/serverapp/sessiondbmodelsprovider.h
    include/hatn/serverapp/sessioncontroller.h
    include/hatn/serverapp/sessiontoken.h
    include/hatn/serverapp/localsessioncontroller.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/serverapp/ipp/localusercontroller.ipp
    include/hatn/serverapp/ipp/localsessioncontroller.ipp
    include/hatn/serverapp/ipp/encryptedtoken.ipp
    include/hatn/serverapp/ipp/sharedsecretprotocol.ipp
)

SET (SOURCES
    src/serverapp.cpp
    src/userdbmodelsprovider.cpp
    src/sharedsecretprotocol.cpp
    src/sessiondbmodelsprovider.cpp
    src/encryptedtoken.cpp
    src/sessiontoken.cpp
    src/localsessioncontroller.cpp
)

BUILD_HATN_MODULE()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC args)

IF (HATN_PLUGIN_rocksdb)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC hatnrocksdbschema)
ENDIF()
