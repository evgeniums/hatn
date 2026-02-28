PROJECT(hatngrpcclient)

FIND_PACKAGE(Protobuf CONFIG REQUIRED)
FIND_PACKAGE(gRPC CONFIG REQUIRED)

SET (HEADERS
    include/hatn/grpcclient/grpcclient.h
    include/hatn/grpcclient/grpctransport.h
    include/hatn/grpcclient/grpcrouter.h
)

SET (SOURCES
    src/grpcclient.cpp
)

BUILD_HATN_MODULE()
