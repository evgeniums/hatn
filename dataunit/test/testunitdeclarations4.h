#pragma once

HDU_UNIT(wire_fixed_uint32_repeated,
            HDU_REPEATED_FIELD(f0,TYPE_FIXED_UINT32,1)
            )
HDU_UNIT(wire_fixed_uint64_repeated,
            HDU_REPEATED_FIELD(f0,TYPE_FIXED_UINT64,1)
            )
HDU_UNIT(wire_fixed_int32_repeated,
            HDU_REPEATED_FIELD(f0,TYPE_FIXED_INT32,1)
            )
HDU_UNIT(wire_fixed_int64_repeated,
            HDU_REPEATED_FIELD(f0,TYPE_FIXED_INT64,1)
            )

HDU_UNIT(wire_float_repeated,
            HDU_REPEATED_FIELD(f0,TYPE_FLOAT,1)
            )

HDU_UNIT(wire_double_repeated,
            HDU_REPEATED_FIELD(f0,TYPE_DOUBLE,1)
            )
HDU_UNIT(wire_double_repeated_proto_packed,
            HDU_REPEATED_FIELD(f0,TYPE_DOUBLE,1,false,Auto,ProtobufPacked)
            )
HDU_UNIT(wire_double_repeated_proto,
            HDU_REPEATED_FIELD(f0,TYPE_DOUBLE,1,false,Auto,ProtobufOrdinary)
            )
