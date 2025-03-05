#pragma once

HDU_UNIT(repeated,
  HDU_REPEATED_FIELD(type_repeated,TYPE_INT32,21)
  HDU_REPEATED_FIELD(type_repeated_pbuf,TYPE_INT32,22,false,Auto,ProtobufUnpacked)
  HDU_REPEATED_FIELD(type_repeated_pbuf_packed,TYPE_INT32,23,false,Auto,ProtobufPacked)
  HDU_REPEATED_FIELD(type_repeated_fixed_str1,HDU_TYPE_FIXED_STRING(8),24)
)

HDU_UNIT(all_types_copy,
   HDU_FIELD(type_bool,TYPE_BOOL,1)
   HDU_FIELD(type_int8,TYPE_INT8,2)
   HDU_FIELD(type_int16,TYPE_INT16,3)
   HDU_FIELD(type_int32,TYPE_INT32,4)
   HDU_FIELD(type_int64,TYPE_INT64,5)
   HDU_FIELD(type_uint8,TYPE_UINT8,6)
   HDU_FIELD(type_uint16,TYPE_UINT16,7)
   HDU_FIELD(type_uint32,TYPE_UINT32,8)
   HDU_FIELD(type_uint64,TYPE_UINT64,9)
   HDU_FIELD(type_float,TYPE_FLOAT,10)
   HDU_FIELD(type_double,TYPE_DOUBLE,11)
   HDU_FIELD(type_string,TYPE_STRING,12)
   HDU_FIELD(type_bytes,TYPE_BYTES,13)
   HDU_FIELD(type_fixed_string,HDU_TYPE_FIXED_STRING(8),20)
   HDU_REQUIRED_FIELD(type_int8_required,TYPE_INT8,25)
)

HDU_UNIT(default_fields,
  HDU_DEFAULT_FIELD(type_bool,TYPE_BOOL,1,true)
  HDU_DEFAULT_FIELD(type_int32,TYPE_INT32,2,1000)
  HDU_DEFAULT_FIELD(type_float,TYPE_FLOAT,3,1010.2150f)
  HDU_DEFAULT_FIELD(type_double,TYPE_DOUBLE,4,3020.1123f)
  HDU_ENUM(MyEnum,One=1,Two=2)
  HDU_DEFAULT_FIELD(type_enum,HDU_TYPE_ENUM(MyEnum),100,MyEnum::Two)
  HDU_REPEATED_FIELD(type_double_repeated,TYPE_DOUBLE,5,false,3030.3f)
  HDU_REPEATED_FIELD(type_enum_repeated,HDU_TYPE_ENUM(MyEnum),101,false,MyEnum::One)
)

HDU_UNIT(wire_bool,
  HDU_FIELD(f0,TYPE_BOOL,1)
)
HDU_UNIT(wire_uint8,
  HDU_FIELD(f0,TYPE_UINT8,1)
)
HDU_UNIT(wire_uint16,
  HDU_FIELD(f0,TYPE_UINT16,1)
)
HDU_UNIT(wire_uint32,
  HDU_FIELD(f0,TYPE_UINT32,1)
)
HDU_UNIT(wire_uint64,
  HDU_FIELD(f0,TYPE_UINT64,1)
)
HDU_UNIT(wire_int8,
  HDU_FIELD(f0,TYPE_INT8,200)
)
HDU_UNIT(wire_int16,
  HDU_FIELD(f0,TYPE_INT16,10)
)
HDU_UNIT(wire_int32,
  HDU_FIELD(f0,TYPE_INT32,3)
)
HDU_UNIT(wire_int64,
  HDU_FIELD(f0,TYPE_INT64,2)
)

HDU_UNIT(wire_fixed_uint32,
  HDU_FIELD(f0,TYPE_FIXED_UINT32,1)
)
HDU_UNIT(wire_fixed_uint64,
  HDU_FIELD(f0,TYPE_FIXED_UINT64,1)
)
HDU_UNIT(wire_fixed_int32,
  HDU_FIELD(f0,TYPE_FIXED_INT32,1)
)
HDU_UNIT(wire_fixed_int64,
  HDU_FIELD(f0,TYPE_FIXED_INT64,1)
)

HDU_UNIT(wire_float,
  HDU_FIELD(f0,TYPE_FLOAT,1)
)
HDU_UNIT(wire_double,
  HDU_FIELD(f0,TYPE_DOUBLE,1)
)

HDU_UNIT(wire_bytes,
  HDU_FIELD(f0,TYPE_BYTES,1)
)
