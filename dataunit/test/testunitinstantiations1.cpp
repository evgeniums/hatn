

#include <hatn/dataunit/syntax.h>
#include <hatn/common/pmr/withstaticallocator.ipp>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

#include "testunitlib.h"
#include "simpleunitdeclaration.h"

HDU_INSTANTIATE(simple_int8,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(all_types,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(simple_subunit,TEST_UNIT_EXPORT)
HDU_INSTANTIATE(names_and_descr,TEST_UNIT_EXPORT)

HDU_V2_EXPORT_EMPTY(empty_unit,TEST_UNIT_EXPORT)
