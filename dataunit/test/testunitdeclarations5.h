#pragma once

HDU_UNIT(wire_bytes_repeated,
            HDU_REPEATED_FIELD(f0,TYPE_BYTES,1)
            )
HDU_UNIT(wire_bytes_repeated_proto,
            HDU_REPEATED_FIELD(f0,TYPE_BYTES,1,false,Auto,ProtobufUnpacked)
            )
HDU_UNIT(wire_string_repeated,
            HDU_REPEATED_FIELD(f0,TYPE_STRING,1)
            )
HDU_UNIT(wire_string_repeated_proto,
            HDU_REPEATED_FIELD(f0,TYPE_STRING,1,false,Auto,ProtobufUnpacked)
            )
HDU_UNIT(wire_fixed_string_repeated,
            HDU_REPEATED_FIELD(f0,HDU_TYPE_FIXED_STRING(128),1)
            )
HDU_UNIT(wire_fixed_string_repeated_proto,
            HDU_REPEATED_FIELD(f0,HDU_TYPE_FIXED_STRING(128),1,false,Auto,ProtobufUnpacked)
            )

HDU_UNIT_WITH(ext0,(HDU_BASE(all_types)),
                 HDU_FIELD(ext0_int32,TYPE_INT32,202)
                 )
HDU_UNIT_WITH(ext1,(HDU_BASE(ext0)),
                 HDU_FIELD(type_bool1,TYPE_BOOL,100)
                 )
HDU_UNIT_WITH(ext2,(HDU_BASE(empty_unit)),
                 HDU_FIELD(type_int8,TYPE_INT8,2)
                 )
