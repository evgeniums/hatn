HDU_V2_UNIT(simple_int8,
  HDU_V2_FIELD(type_int8,TYPE_INT8,2)
)

HDU_V2_UNIT(all_types,
    HDU_V2_FIELD(type_bool,TYPE_BOOL,1)
    HDU_V2_FIELD(type_int8,TYPE_INT8,2)
    HDU_V2_FIELD(type_int16,TYPE_INT16,3)
    HDU_V2_FIELD(type_int32,TYPE_INT32,4)
    HDU_V2_FIELD(type_int64,TYPE_INT64,5)
    HDU_V2_FIELD(type_uint8,TYPE_UINT8,6)
    HDU_V2_FIELD(type_uint16,TYPE_UINT16,7)
    HDU_V2_FIELD(type_uint32,TYPE_UINT32,8)
    HDU_V2_FIELD(type_uint64,TYPE_UINT64,9)
    HDU_V2_FIELD(type_float,TYPE_FLOAT,10)
    HDU_V2_FIELD(type_double,TYPE_DOUBLE,11)
    HDU_V2_FIELD(type_string,TYPE_STRING,12)
    HDU_V2_FIELD(type_bytes,TYPE_BYTES,13)
    HDU_V2_FIELD(type_fixed_string,HDU_V2_TYPE_FIXED_STRING(8),20)
    HDU_V2_REQUIRED_FIELD(type_int8_required,TYPE_INT8,25)
    HDU_V2_ENUM(MyEnum,One=1,Two=2)
)

HDU_V2_UNIT(simple_subunit,
    HDU_V2_FIELD(f0,simple_int8::TYPE,1)
)

HDU_V2_UNIT(names_and_descr,
    HDU_V2_FIELD(optional_field,TYPE_INT8,1)
    HDU_V2_FIELD(optional_field_descr,TYPE_INT8,2)

    HDU_V2_REQUIRED_FIELD(required_field,TYPE_INT8,3)
    HDU_V2_REQUIRED_FIELD(required_field_descr,TYPE_INT8,4)

    HDU_V2_DEFAULT_FIELD(default_field,TYPE_INT8,5,1)
    HDU_V2_DEFAULT_FIELD(default_field_descr,TYPE_INT8,6,1)

    HDU_V2_REPEATED_FIELD(repeated_field,TYPE_INT8,7)
    HDU_V2_REPEATED_FIELD(repeated_field_descr,TYPE_INT8,8)
)

HDU_V2_UNIT_EMPTY(empty_unit)
