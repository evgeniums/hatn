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

HDU_INSTANTIATE_DATAUNIT(wire_fixed_uint32_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_uint64_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_int32_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_int64_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_float_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_double_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_double_repeated_proto_packed)
HDU_INSTANTIATE_DATAUNIT(wire_double_repeated_proto)
#endif
