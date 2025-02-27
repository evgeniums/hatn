PROJECT(hatnapi)

SET (HEADERS
    include/hatn/api/api.h
    include/hatn/api/apiconstants.h
    include/hatn/api/apierror.h
    include/hatn/api/apierrorcodes.h
    include/hatn/api/connection.h
    include/hatn/api/connectionpool.h
    include/hatn/api/protocolunits.h
)

SET (HEADERS
    ${HEADERS}

    include/hatn/api/client/client.h
    include/hatn/api/client/router.h
    include/hatn/api/client/resolver.h
    include/hatn/api/client/iphostresolver.h
    include/hatn/api/client/tcpclient.h
    include/hatn/api/client/plaintcpconnection.h
)

SET (HEADERS
    ${HEADERS}

    include/hatn/api/server/dispatcher.h
    include/hatn/api/server/tcpserver.h
    include/hatn/api/server/plaintcpserver.h
)

SET (SOURCES
    src/apierror.cpp
    src/plaintcpclient.cpp
    src/plaintcpserver.cpp
)

BUILD_HATN_MODULE()
