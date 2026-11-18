PROJECT(hatngrpcclient)

FIND_PACKAGE(Protobuf CONFIG REQUIRED)
FIND_PACKAGE(gRPC CONFIG REQUIRED)
# abseil log sink registry is used directly by grpclog.cpp; gRPC's own config already pulls abseil in,
# the explicit lookup only makes the dependency visible here.
FIND_PACKAGE(absl CONFIG REQUIRED)

SET (HEADERS
    include/hatn/grpcclient/grpcclientdefs.h
    include/hatn/grpcclient/grpcclient.h
    include/hatn/grpcclient/grpctransport.h
    include/hatn/grpcclient/grpcrouter.h
    include/hatn/grpcclient/grpclog.h

    include/hatn/grpcclient/ipp/grpctransport.ipp
)

SET (SOURCES
    src/grpcclient.cpp
    src/grpctransport.cpp
    src/grpcstream.cpp
    src/grpclog.cpp

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

# abseil log API used by the log sink (grpclog.cpp); PRIVATE for the same reason as gRPC above.
TARGET_LINK_LIBRARIES(hatngrpcclient PRIVATE
    absl::log_sink_registry
    absl::log_initialize
    absl::log_globals
    absl::log_entry
    absl::log_sink
)
