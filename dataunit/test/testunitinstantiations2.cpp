
#include <hatn/dataunit/syntax.h>
#include <hatn/common/pmr/withstaticallocator.ipp>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

#include "testunitlib.h"
#include "testunitdeclarations2.h"

HDU_INSTANTIATE(repeated,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(all_types_copy,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(default_fields,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_bool,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_uint8,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_uint16,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_uint32,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_uint64,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_int8,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_int16,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_int32,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_int64,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_fixed_uint32,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_fixed_uint64,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_fixed_int32,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_fixed_int64,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_float,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_double,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_bytes,TEST_UNIT_EXPORT)
