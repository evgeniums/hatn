PROJECT(hatnnetwork)

SET (HEADERS
    include/hatn/network/network.h
    include/hatn/network/networkerror.h
    include/hatn/network/networkerrorcodes.h
    include/hatn/network/endpoint.h
    include/hatn/network/ipendpoint.h
    include/hatn/network/reliablestream.h
    include/hatn/network/unreliablechannel.h

    include/hatn/network/resolver.h
    include/hatn/network/resolvershuffle.h

    include/hatn/network/asio/ipendpoint.h
    include/hatn/network/asio/socket.h
    include/hatn/network/asio/tcpstream.h
    include/hatn/network/asio/tcpserver.h
    include/hatn/network/asio/tcpserverconfig.h
    include/hatn/network/asio/udpchannel.h
    include/hatn/network/asio/caresolver.h

    include/hatn/network/detail/asynchandler.ipp

#    include/hatn/network/resolverinterface.h

    include/hatn/network/socks5client.h
)

SET (SOURCES
    src/network.cpp
    src/error.cpp

    # src/resolver.cpp
    # src/resolvershuffle.cpp

    src/asio/tcpstream.cpp
    src/asio/tcpserver.cpp
    src/asio/udpchannel.cpp
    # src/asio/caresolver.cpp

#    src/resolverinterface.cpp

    # src/socks5client.cpp
)

BUILD_HATN_MODULE()

IF (NOT WIN32)
    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC -DHAVE_ARES_CONFIG_H)
ENDIF()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC cares)

IF(WIN32)
IF(NOT MSVC)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC ws2_32 wsock32 iphlpapi uuid rpcrt4 winmm ole32)
ENDIF(NOT MSVC)
ENDIF(WIN32)
