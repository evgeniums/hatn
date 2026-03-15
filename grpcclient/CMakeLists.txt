PROJECT(hatngrpcclient)

FIND_PACKAGE(Protobuf CONFIG REQUIRED)
FIND_PACKAGE(gRPC CONFIG REQUIRED)

SET (HEADERS
    include/hatn/grpcclient/grpcclientdefs.h
    include/hatn/grpcclient/grpcclient.h
    include/hatn/grpcclient/grpctransport.h
    include/hatn/grpcclient/grpcrouter.h
    include/hatn/grpcclient/grpcstream.h

    include/hatn/grpcclient/ipp/grpctransport.ipp
)

SET (SOURCES
    src/grpctransport.cpp
    src/grpcstream.cpp
)

BUILD_HATN_MODULE()

TARGET_LINK_LIBRARIES(hatngrpcclient PUBLIC gRPC::grpc++)
