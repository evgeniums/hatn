#include <hatn/dataunit/types.h>
using namespace HATN_DATAUNIT_NAMESPACE::types;

#define HATN_WITH_STATIC_ALLOCATOR_SRC
#include "testunitdeclarations.h"
#undef HATN_WITH_STATIC_ALLOCATOR_SRC

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/syntax.ipp>

HDU_INSTANTIATE_DATAUNIT(names_and_descr)
HDU_INSTANTIATE_DATAUNIT(subunit_names_and_descr)

HDU_INSTANTIATE_DATAUNIT(simple_int8)
HDU_INSTANTIATE_DATAUNIT(all_types)

#if 1

#if 0
HDU_INSTANTIATE_DATAUNIT(many_fields)
#endif

HDU_INSTANTIATE_DATAUNIT(all_types_copy)

HDU_INSTANTIATE_DATAUNIT_EXT(all_types,ext0)
HDU_INSTANTIATE_DATAUNIT_EXT(all_types,ext1)

HDU_INSTANTIATE_DATAUNIT(repeated)

HDU_INSTANTIATE_DATAUNIT(with_unit)
HDU_INSTANTIATE_DATAUNIT(with_unit2)
HDU_INSTANTIATE_DATAUNIT(with_unit3)
HDU_INSTANTIATE_DATAUNIT(with_unit4)
HDU_INSTANTIATE_DATAUNIT(wire_unit_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_embedded_repeated)
HDU_INSTANTIATE_DATAUNIT(embedded_unit)
HDU_INSTANTIATE_DATAUNIT(shared_unit)
HDU_INSTANTIATE_DATAUNIT(tree)

HDU_INSTANTIATE_DATAUNIT(wire_simple_unit_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_unit_repeated1)
HDU_INSTANTIATE_DATAUNIT(wire_unit_repeated_protobuf1)

HDU_INSTANTIATE_DATAUNIT(wire_embedded_uint8_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_embedded_all_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_embedded_all_repeated_protobuf)
HDU_INSTANTIATE_DATAUNIT(wire_external_all_repeated_protobuf)

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
HDU_INSTANTIATE_DATAUNIT(wire_uint8_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_uint16_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_uint32_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_uint64_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_int8_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_int16_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_int32_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_int64_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_uint32_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_uint64_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_int32_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_int64_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_float_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_double_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_double_repeated_proto_packed)
HDU_INSTANTIATE_DATAUNIT(wire_double_repeated_proto)
HDU_INSTANTIATE_DATAUNIT(wire_bytes_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_bytes_repeated_proto)
HDU_INSTANTIATE_DATAUNIT(wire_string_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_string_repeated_proto)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_string_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_string_repeated_proto)
#endif
