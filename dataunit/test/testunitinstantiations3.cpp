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

HDU_INSTANTIATE_DATAUNIT(empty_unit)
HDU_INSTANTIATE_DATAUNIT_EXT(empty_unit,ext2)
HDU_INSTANTIATE_DATAUNIT(default_fields)
HDU_INSTANTIATE_DATAUNIT(wire_bool)
HDU_INSTANTIATE_DATAUNIT(wire_uint8)
HDU_INSTANTIATE_DATAUNIT(wire_uint16)
HDU_INSTANTIATE_DATAUNIT(wire_uint32)
HDU_INSTANTIATE_DATAUNIT(wire_uint64)
HDU_INSTANTIATE_DATAUNIT(wire_int8)
HDU_INSTANTIATE_DATAUNIT(wire_int16)
HDU_INSTANTIATE_DATAUNIT(wire_int32)
HDU_INSTANTIATE_DATAUNIT(wire_int64)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_uint32)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_uint64)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_int32)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_int64)
HDU_INSTANTIATE_DATAUNIT(wire_float)
HDU_INSTANTIATE_DATAUNIT(wire_double)
HDU_INSTANTIATE_DATAUNIT(wire_bytes)

#endif
