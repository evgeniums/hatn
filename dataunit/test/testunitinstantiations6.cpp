
#include <hatn/dataunit/unitmacros.h>
#include <hatn/common/pmr/withstaticallocator.ipp>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>

#include "simpleunitdeclaration.h"
#include "testunitlib.h"
#include "testunitdeclarations6.h"

HDU_V2_INSTANTIATE(subunit_names_and_descr,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(embedded_unit,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(shared_unit,TEST_UNIT_EXPORT)
HDU_V2_INSTANTIATE(with_unit,TEST_UNIT_EXPORT)
