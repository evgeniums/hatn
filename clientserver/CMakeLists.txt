PROJECT(hatnclientserver)

SET (HEADERS
    include/hatn/clientserver/clientserver.h
    include/hatn/clientserver/clientservererror.h
    include/hatn/clientserver/clientservererrorcodes.h
    include/hatn/clientserver/usercontroller.h
    include/hatn/clientserver/accountconfigparser.h

    include/hatn/clientserver/auth/authprotocol.h
    include/hatn/clientserver/auth/clientauthprotocol.h
    include/hatn/clientserver/auth/clientauthprotocolsharedsecret.h
    include/hatn/clientserver/auth/defaultauth.h
    include/hatn/clientserver/auth/clientsession.h
    include/hatn/clientserver/auth/clientsessionsharedsecret.h

    include/hatn/clientserver/clientwithauth.h
    include/hatn/clientserver/plaintcpclientwithauth.h

    include/hatn/clientserver/models/genericdbmodels.h

    include/hatn/clientserver/models/cache.h
    include/hatn/clientserver/models/cachedbmodel.h

    include/hatn/clientserver/cacheoptions.h
    include/hatn/clientserver/objectscache.h
    include/hatn/clientserver/models/expire.h
)

SET (HEADERS
    ${HEADERS}

    include/hatn/clientserver/ipp/clientauthprotocolsharedsecret.ipp
    include/hatn/clientserver/ipp/clientsession.ipp
    include/hatn/clientserver/ipp/clientwithauth.ipp

    include/hatn/clientserver/ipp/objectscache.ipp
)

SET (HEADERS
    ${HEADERS}

    include/hatn/clientserver/models/oid.h
    include/hatn/clientserver/models/revision.h
    include/hatn/clientserver/models/encrypted.h
    include/hatn/clientserver/models/name.h
    include/hatn/clientserver/models/username.h
    include/hatn/clientserver/models/addressitem.h
    include/hatn/clientserver/models/employment.h
    include/hatn/clientserver/models/notifications.h
    include/hatn/clientserver/models/unreadcount.h
    include/hatn/clientserver/models/user.h
    include/hatn/clientserver/models/withuser.h
    include/hatn/clientserver/models/withpassphrase.h
    include/hatn/clientserver/models/loginprofile.h
    include/hatn/clientserver/models/usercharacter.h
    include/hatn/clientserver/models/withusercharacter.h
    include/hatn/clientserver/models/withloginprofile.h
    include/hatn/clientserver/models/serverhost.h
    include/hatn/clientserver/models/invitation.h
    include/hatn/clientserver/models/usernamevalidator.h

    include/hatn/clientserver/models/serverroute.h
    include/hatn/clientserver/models/accountconfig.h
)

SET (HEADERS
    ${HEADERS}

    include/hatn/clientserver/messages/accountmessages.h
)

SET (SOURCES
    src/clientserver.cpp
    src/accountconfigparser.cpp
    src/clientservererror.cpp
    src/defaultauth.cpp
    src/clientsession.cpp

    src/clientauthprotocolsharedsecret.cpp
    src/clientsessionsharedsecret.cpp
    src/plaintcpclientwithauth.cpp

    src/cachedbmodelsprovider.cpp
)

BUILD_HATN_MODULE()

IF(MSVC)
    TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PRIVATE /bigobj)
ENDIF()

IF (HATN_PLUGIN_rocksdb)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC hatnrocksdbschema)
ENDIF()
