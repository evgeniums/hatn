PROJECT(hatnapi)

SET (HEADERS
    include/hatn/api/api.h
    include/hatn/api/apiconstants.h
    include/hatn/api/apierror.h
    include/hatn/api/apierrorcodes.h
    include/hatn/api/connection.h
    include/hatn/api/connectionpool.h
    include/hatn/api/authunit.h
    include/hatn/api/requestunit.h
    include/hatn/api/responseunit.h
    include/hatn/api/method.h
    include/hatn/api/service.h
    include/hatn/api/priority.h
    include/hatn/api/typealiases.h
)

SET (HEADERS
    ${HEADERS}

    include/hatn/api/client/client.h
    include/hatn/api/client/router.h
    include/hatn/api/client/resolver.h
    include/hatn/api/client/iphostresolver.h
    include/hatn/api/client/tcpclient.h
    include/hatn/api/client/plaintcpconnection.h
    include/hatn/api/client/request.h
    include/hatn/api/client/session.h
)

SET (HEADERS
    ${HEADERS}

    include/hatn/api/server/dispatcher.h
    include/hatn/api/server/tcpserver.h
    include/hatn/api/server/plaintcpserver.h
)

SET (IPP_HEADERS
    include/hatn/api/ipp/request.ipp
    include/hatn/api/ipp/client.ipp
)

SET (HEADERS
    ${HEADERS}
    ${IPP_HEADERS}
)

SET (SOURCES
    src/apierror.cpp
    src/plaintcpclient.cpp
    src/plaintcpserver.cpp
)

BUILD_HATN_MODULE()
