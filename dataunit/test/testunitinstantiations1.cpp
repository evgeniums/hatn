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

#endif
