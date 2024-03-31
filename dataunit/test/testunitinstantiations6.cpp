#include <hatn/dataunit/types.h>
using namespace HATN_DATAUNIT_NAMESPACE::types;

#define HATN_WITH_STATIC_ALLOCATOR_SRC
#include "testunitdeclarations.h"
#undef HATN_WITH_STATIC_ALLOCATOR_SRC

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/syntax.ipp>

#if 1

HDU_INSTANTIATE_DATAUNIT(wire_bytes_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_bytes_repeated_proto)
HDU_INSTANTIATE_DATAUNIT(wire_string_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_string_repeated_proto)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_string_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_fixed_string_repeated_proto)

#endif
