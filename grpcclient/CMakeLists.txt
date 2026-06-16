PROJECT(hatngrpcclient)

FIND_PACKAGE(Protobuf CONFIG REQUIRED)
FIND_PACKAGE(gRPC CONFIG REQUIRED)

SET (HEADERS
    include/hatn/grpcclient/grpcclientdefs.h
    include/hatn/grpcclient/grpcclient.h
    include/hatn/grpcclient/grpctransport.h
    include/hatn/grpcclient/grpcrouter.h

    include/hatn/grpcclient/ipp/grpctransport.ipp
)

SET (SOURCES
    src/grpctransport.cpp
    src/grpcstream.cpp

    # DLL-private headers: keep gRPC/abseil/protobuf fully encapsulated inside
    # hatngrpcclient.dll. Listed here (not in HEADERS) so they are tracked but
    # NOT installed as public headers — consumers never see a gRPC type.
    src/grpcstream.h
    src/grpctransport_p.h
)

BUILD_HATN_MODULE()

# PRIVATE: gRPC must not propagate to consumers. The public header layer
# (grpctransport.h + grpctransport.ipp) is gRPC-free and hands off to the
# exported GrpcTransport::*Impl methods, which own every gRPC type. Linking
# gRPC PRIVATE keeps a single gRPC runtime (one ExecCtx) inside this DLL and
# stops gRPC being statically duplicated into consumer modules.
TARGET_LINK_LIBRARIES(hatngrpcclient PRIVATE gRPC::grpc++)
