
#include <hatn/dataunit/unitmacros.h>
#include <hatn/common/pmr/withstaticallocator.ipp>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>

#include "simpleunitdeclaration.h"
#include "testunitdeclarations2.h"

#include "testunitlib.h"
#include "testunitdeclarations8.h"

HDU_V2_INSTANTIATE(wire_unit_repeated,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_embedded_repeated,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_embedded_uint8_repeated,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_embedded_all_repeated,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_embedded_all_repeated_protobuf,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_external_all_repeated_protobuf,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_simple_unit_repeated,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_unit_repeated1,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(wire_unit_repeated_protobuf1,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(tree,TEST_UNIT_EXPORT)
