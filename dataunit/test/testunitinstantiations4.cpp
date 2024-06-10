
#include <hatn/dataunit/syntax.h>
#include <hatn/common/pmr/withstaticallocator.ipp>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

#include "testunitlib.h"
#include "testunitdeclarations4.h"

HDU_INSTANTIATE(wire_fixed_uint32_repeated,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_fixed_uint64_repeated,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_fixed_int32_repeated,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_fixed_int64_repeated,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_float_repeated,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_double_repeated,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_double_repeated_proto_packed,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_double_repeated_proto,TEST_UNIT_EXPORT)
