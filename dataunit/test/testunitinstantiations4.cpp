
#include <hatn/dataunit/unitmacros.h>
#include <hatn/common/pmr/withstaticallocator.ipp>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>

#include "testunitlib.h"
#include "testunitdeclarations4.h"

HDU_V2_INSTANTIATE(wire_fixed_uint32_repeated,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_fixed_uint64_repeated,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_fixed_int32_repeated,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_fixed_int64_repeated,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_float_repeated,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_double_repeated,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_double_repeated_proto_packed,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_double_repeated_proto,TEST_UNIT_EXPORT)
