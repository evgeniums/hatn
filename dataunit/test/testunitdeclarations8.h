#pragma once

HDU_UNIT(wire_unit_repeated,
            HDU_REPEATED_FIELD(f0,all_types::TYPE,1,false,Auto,Auto,External)
            )

HDU_UNIT(wire_embedded_repeated,
            HDU_REPEATED_FIELD(f0,empty_unit::TYPE,1,false,Auto,Auto,Embedded)
            )

HDU_UNIT(wire_embedded_uint8_repeated,
            HDU_REPEATED_FIELD(f0,wire_uint8::TYPE,1,false,Auto,Auto,Embedded)
            )

HDU_UNIT(wire_embedded_all_repeated,
            HDU_REPEATED_FIELD(f0,all_types::TYPE,1,false,Auto,Auto,Embedded)
            )

HDU_UNIT(wire_embedded_all_repeated_protobuf,
            HDU_REPEATED_FIELD(f0,all_types::TYPE,1,false,Auto,ProtobufUnpacked,Embedded)
            )
HDU_UNIT(wire_external_all_repeated_protobuf,
            HDU_REPEATED_FIELD(f0,all_types::TYPE,1,false,Auto,ProtobufUnpacked,External)
            )

HDU_UNIT(wire_simple_unit_repeated,
            HDU_REPEATED_FIELD(f0,simple_int8::TYPE,1,false,Auto,Auto,External)
            )

HDU_UNIT(wire_unit_repeated1,
            HDU_REPEATED_FIELD(f0,all_types::TYPE,1)
            )
HDU_UNIT(wire_unit_repeated_protobuf1,
            HDU_REPEATED_FIELD(f0,all_types::TYPE,1)
            )

HDU_UNIT(tree,
            HDU_DEFAULT_FIELD(f0,TYPE_INT32,1,5)
            HDU_REPEATED_FIELD(children,TYPE_DATAUNIT,2,false,Auto,Auto,External)
            )
