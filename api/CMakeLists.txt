PROJECT(hatnapi)

SET (HEADERS
    include/hatn/api/api.h
    include/hatn/api/apiconstants.h
    include/hatn/api/protocol.h
    include/hatn/api/apiliberror.h
    include/hatn/api/apiliberrorcodes.h
    include/hatn/api/connection.h
    include/hatn/api/connectionpool.h
    include/hatn/api/authunit.h
    include/hatn/api/requestunit.h
    include/hatn/api/responseunit.h
    include/hatn/api/method.h
    include/hatn/api/service.h
    include/hatn/api/priority.h
    include/hatn/api/typealiases.h
    include/hatn/api/router.h
    include/hatn/api/message.h
    include/hatn/api/auth.h
    include/hatn/api/tenancy.h
    include/hatn/api/withnameandversion.h
)

SET (HEADERS
    ${HEADERS}

    include/hatn/api/client/client.h    
    include/hatn/api/client/resolver.h
    include/hatn/api/client/iphostresolver.h
    include/hatn/api/client/tcpclient.h
    include/hatn/api/client/plaintcpconnection.h
    include/hatn/api/client/plaintcprouter.h
    include/hatn/api/client/request.h
    include/hatn/api/client/response.h
    include/hatn/api/client/session.h
    include/hatn/api/client/methodauth.h
)

SET (HEADERS
    ${HEADERS}

    include/hatn/api/server/dispatcher.h
    include/hatn/api/server/tcpserver.h
    include/hatn/api/server/plaintcpserver.h
    include/hatn/api/server/serverrequest.h
    include/hatn/api/server/serverresponse.h
    include/hatn/api/server/servicerouter.h
    include/hatn/api/server/topicrouter.h
    include/hatn/api/server/server.h
    include/hatn/api/server/env.h
    include/hatn/api/server/requestrouter.h
    include/hatn/api/server/servicedispatcher.h
    include/hatn/api/server/connectionsstore.h
    include/hatn/api/server/serverservice.h
)

SET (IPP_HEADERS
    include/hatn/api/ipp/request.ipp
    include/hatn/api/ipp/client.ipp
    include/hatn/api/ipp/session.ipp
    include/hatn/api/ipp/message.ipp
    include/hatn/api/ipp/auth.ipp
    include/hatn/api/ipp/methodauth.ipp
    include/hatn/api/ipp/serverservice.ipp
    include/hatn/api/ipp/serverresponse.ipp
)

SET (HEADERS
    ${HEADERS}
    ${IPP_HEADERS}
)

SET (SOURCES
    src/apiliberror.cpp
    src/plaintcpclient.cpp
    src/plaintcpserver.cpp
)

BUILD_HATN_MODULE()
