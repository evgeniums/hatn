#include <hatn/dataunit/types.h>
using namespace HATN_DATAUNIT_NAMESPACE::types;

#define HATN_WITH_STATIC_ALLOCATOR_SRC
#include "testunitdeclarations.h"
#undef HATN_WITH_STATIC_ALLOCATOR_SRC

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/syntax.ipp>

#if 1

#if 0
HDU_INSTANTIATE_DATAUNIT(many_fields)
#endif

HDU_INSTANTIATE_DATAUNIT(wire_simple_unit_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_unit_repeated1)
HDU_INSTANTIATE_DATAUNIT(wire_unit_repeated_protobuf1)

HDU_INSTANTIATE_DATAUNIT(wire_embedded_uint8_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_embedded_all_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_embedded_all_repeated_protobuf)
HDU_INSTANTIATE_DATAUNIT(wire_external_all_repeated_protobuf)

#endif
