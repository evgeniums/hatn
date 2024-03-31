#include <hatn/dataunit/types.h>
using namespace HATN_DATAUNIT_NAMESPACE::types;

#define HATN_WITH_STATIC_ALLOCATOR_SRC
#include "testunitdeclarations.h"
#undef HATN_WITH_STATIC_ALLOCATOR_SRC

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/syntax.ipp>

#if 1

#if 0
HDU_INSTANTIATE_DATAUNIT(many_fields)
#endif

HDU_INSTANTIATE_DATAUNIT(wire_uint8_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_uint16_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_uint32_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_uint64_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_int8_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_int16_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_int32_repeated)
HDU_INSTANTIATE_DATAUNIT(wire_int64_repeated)
#endif
