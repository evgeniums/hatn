
#include <hatn/dataunit/syntax.h>
#include <hatn/common/pmr/withstaticallocator.ipp>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

#include "simpleunitdeclaration.h"
#include "testunitlib.h"
#include "testunitdeclarations5.h"

HDU_INSTANTIATE(wire_bytes_repeated,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_bytes_repeated_proto,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_string_repeated,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_string_repeated_proto,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_fixed_string_repeated,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(wire_fixed_string_repeated_proto,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(ext0,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(ext1,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(ext2,TEST_UNIT_EXPORT)
