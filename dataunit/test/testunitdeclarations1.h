#pragma once

#ifdef WIN32
#    ifndef TEST_UNIT_EXPORT
#        ifdef BUILD_TEST_UNIT
#            define TEST_UNIT_EXPORT __declspec(dllexport)
#        else
#            define TEST_UNIT_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define TEST_UNIT_EXPORT
#endif

#include <hatn/dataunit/unitmacros.h>

#ifdef _WIN32
#define HDU_V2_UNIT_EXPORT TEST_UNIT_EXPORT
#include <hatn/dataunit/unitdll.h>
#endif

HDU_V2_UNIT(simple_int8,
  HDU_V2_FIELD(type_int8,TYPE_INT8,2)
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

HDU_V2_UNIT(repeated,
  HDU_V2_REPEATED_FIELD(type_repeated,TYPE_INT32,21)
  HDU_V2_REPEATED_FIELD(type_repeated_pbuf,TYPE_INT32,22,false,Auto,ProtobufOrdinary)
  HDU_V2_REPEATED_FIELD(type_repeated_pbuf_packed,TYPE_INT32,23,false,Auto,ProtobufPacked)
  HDU_V2_REPEATED_FIELD(type_repeated_fixed_str1,HDU_V2_TYPE_FIXED_STRING(8),24)
)

HDU_V2_UNIT(all_types_copy,
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
)

HDU_V2_UNIT_EMPTY(empty_unit)

HDU_V2_UNIT(default_fields,
  HDU_V2_DEFAULT_FIELD(type_bool,TYPE_BOOL,1,true)
  HDU_V2_DEFAULT_FIELD(type_int32,TYPE_INT32,2,1000)
  HDU_V2_DEFAULT_FIELD(type_float,TYPE_FLOAT,3,1010.2150f)
  HDU_V2_DEFAULT_FIELD(type_double,TYPE_DOUBLE,4,3020.1123f)
  HDU_V2_ENUM(MyEnum,One=1,Two=2)
  HDU_V2_DEFAULT_FIELD(type_enum,HDU_V2_TYPE_ENUM(MyEnum),100,MyEnum::Two)
  HDU_V2_REPEATED_FIELD(type_double_repeated,TYPE_DOUBLE,5,false,3030.3f)
  HDU_V2_REPEATED_FIELD(type_enum_repeated,HDU_V2_TYPE_ENUM(MyEnum),101,false,MyEnum::One)
)

HDU_V2_UNIT(wire_bool,
  HDU_V2_FIELD(f0,TYPE_BOOL,1)
)
HDU_V2_UNIT(wire_uint8,
  HDU_V2_FIELD(f0,TYPE_UINT8,1)
)
HDU_V2_UNIT(wire_uint16,
  HDU_V2_FIELD(f0,TYPE_UINT16,1)
)
HDU_V2_UNIT(wire_uint32,
  HDU_V2_FIELD(f0,TYPE_UINT32,1)
)
HDU_V2_UNIT(wire_uint64,
  HDU_V2_FIELD(f0,TYPE_UINT64,1)
)
HDU_V2_UNIT(wire_int8,
  HDU_V2_FIELD(f0,TYPE_INT8,200)
)
HDU_V2_UNIT(wire_int16,
  HDU_V2_FIELD(f0,TYPE_INT16,10)
)
HDU_V2_UNIT(wire_int32,
  HDU_V2_FIELD(f0,TYPE_INT32,3)
)
HDU_V2_UNIT(wire_int64,
  HDU_V2_FIELD(f0,TYPE_INT64,2)
)

HDU_V2_UNIT(wire_fixed_uint32,
  HDU_V2_FIELD(f0,TYPE_FIXED_UINT32,1)
)
HDU_V2_UNIT(wire_fixed_uint64,
  HDU_V2_FIELD(f0,TYPE_FIXED_UINT64,1)
)
HDU_V2_UNIT(wire_fixed_int32,
  HDU_V2_FIELD(f0,TYPE_FIXED_INT32,1)
)
HDU_V2_UNIT(wire_fixed_int64,
  HDU_V2_FIELD(f0,TYPE_FIXED_INT64,1)
)

HDU_V2_UNIT(wire_float,
  HDU_V2_FIELD(f0,TYPE_FLOAT,1)
)
HDU_V2_UNIT(wire_double,
  HDU_V2_FIELD(f0,TYPE_DOUBLE,1)
)

HDU_V2_UNIT(wire_bytes,
  HDU_V2_FIELD(f0,TYPE_BYTES,1)
)

HDU_V2_UNIT(wire_uint8_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_UINT8,1)
)
HDU_V2_UNIT(wire_uint16_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_UINT16,1)
)
HDU_V2_UNIT(wire_uint32_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_UINT32,1)
)
HDU_V2_UNIT(wire_uint64_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_UINT64,1)
)
HDU_V2_UNIT(wire_int8_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_INT8,200)
)
HDU_V2_UNIT(wire_int16_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_INT16,10)
)
HDU_V2_UNIT(wire_int32_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_INT32,3)
)
HDU_V2_UNIT(wire_int64_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_INT64,2)
)

HDU_V2_UNIT(wire_fixed_uint32_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_FIXED_UINT32,1)
)
HDU_V2_UNIT(wire_fixed_uint64_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_FIXED_UINT64,1)
)
HDU_V2_UNIT(wire_fixed_int32_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_FIXED_INT32,1)
)
HDU_V2_UNIT(wire_fixed_int64_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_FIXED_INT64,1)
)

HDU_V2_UNIT(wire_float_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_FLOAT,1)
)

HDU_V2_UNIT(wire_double_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_DOUBLE,1)
)
HDU_V2_UNIT(wire_double_repeated_proto_packed,
  HDU_V2_REPEATED_FIELD(f0,TYPE_DOUBLE,1,false,Auto,ProtobufPacked)
)
HDU_V2_UNIT(wire_double_repeated_proto,
  HDU_V2_REPEATED_FIELD(f0,TYPE_DOUBLE,1,false,Auto,ProtobufOrdinary)
)

HDU_V2_UNIT(wire_bytes_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_BYTES,1)
)
HDU_V2_UNIT(wire_bytes_repeated_proto,
  HDU_V2_REPEATED_FIELD(f0,TYPE_BYTES,1,false,Auto,ProtobufOrdinary)
)
HDU_V2_UNIT(wire_string_repeated,
  HDU_V2_REPEATED_FIELD(f0,TYPE_STRING,1)
)
HDU_V2_UNIT(wire_string_repeated_proto,
  HDU_V2_REPEATED_FIELD(f0,TYPE_STRING,1,false,Auto,ProtobufOrdinary)
)
HDU_V2_UNIT(wire_fixed_string_repeated,
  HDU_V2_REPEATED_FIELD(f0,HDU_V2_TYPE_FIXED_STRING(128),1)
)
HDU_V2_UNIT(wire_fixed_string_repeated_proto,
  HDU_V2_REPEATED_FIELD(f0,HDU_V2_TYPE_FIXED_STRING(128),1,false,Auto,ProtobufOrdinary)
)

#ifdef _WIN32
#define HDU_V2_UNIT_EXPORT_SHARED TEST_UNIT_EXPORT
#include <hatn/dataunit/unitdll.h>
#endif

HDU_V2_UNIT(subunit_names_and_descr,
            HDU_V2_FIELD(dataunit_field,names_and_descr::TYPE,1)
            HDU_V2_FIELD(dataunit_field_descr,names_and_descr::TYPE,2)
            HDU_V2_FIELD(external_field,names_and_descr::TYPE,3)
            HDU_V2_FIELD(external_field_descr,names_and_descr::TYPE,4)
            HDU_V2_FIELD(embedded_field,names_and_descr::TYPE,5)
            HDU_V2_FIELD(embedded_field_descr,names_and_descr::TYPE,6)
            )

HDU_V2_UNIT(embedded_unit,
            HDU_V2_FIELD(f0,all_types::TYPE,1)
            )
HDU_V2_UNIT(shared_unit,
            HDU_V2_FIELD(f0,all_types::TYPE,1)
            )

HDU_V2_UNIT(with_unit,
            HDU_V2_FIELD(f0,all_types::TYPE,1)
            )

HDU_V2_UNIT_WITH(ext0,(HDU_V2_BASE(all_types)),
                 HDU_V2_FIELD(ext0_int32,TYPE_INT32,202)
                 )
HDU_V2_UNIT_WITH(ext1,(HDU_V2_BASE(ext0)),
                 HDU_V2_FIELD(type_bool1,TYPE_BOOL,100)
                 )

HDU_V2_UNIT_WITH(ext2,(HDU_V2_BASE(empty_unit)),
                 HDU_V2_FIELD(type_int8,TYPE_INT8,2)
                 )

HDU_V2_UNIT(with_unit2,
            HDU_V2_FIELD(f0,simple_int8::TYPE,1)
            )
HDU_V2_UNIT(with_unit3,
            HDU_V2_FIELD(f0,with_unit2::TYPE,1)
            )
HDU_V2_UNIT(with_unit4,
            HDU_V2_FIELD(f0,with_unit3::TYPE,1)
            HDU_V2_FIELD(f1,with_unit2::TYPE,2)
            )

HDU_V2_UNIT(wire_unit_repeated,
  HDU_V2_REPEATED_FIELD(f0,all_types::TYPE,1,false,Auto,Auto,External)
)

HDU_V2_UNIT(wire_embedded_repeated,
  HDU_V2_REPEATED_FIELD(f0,empty_unit::TYPE,1,false,Auto,Auto,Embedded)
)

HDU_V2_UNIT(wire_embedded_uint8_repeated,
  HDU_V2_REPEATED_FIELD(f0,wire_uint8::TYPE,1,false,Auto,Auto,Embedded)
)

HDU_V2_UNIT(wire_embedded_all_repeated,
  HDU_V2_REPEATED_FIELD(f0,all_types::TYPE,1,false,Auto,Auto,Embedded)
)

HDU_V2_UNIT(wire_embedded_all_repeated_protobuf,
  HDU_V2_REPEATED_FIELD(f0,all_types::TYPE,1,false,Auto,ProtobufOrdinary,Embedded)
)
HDU_V2_UNIT(wire_external_all_repeated_protobuf,
  HDU_V2_REPEATED_FIELD(f0,all_types::TYPE,1,false,Auto,ProtobufOrdinary,External)
)

HDU_V2_UNIT(wire_simple_unit_repeated,
  HDU_V2_REPEATED_FIELD(f0,simple_int8::TYPE,1,false,Auto,Auto,External)
)

HDU_V2_UNIT(wire_unit_repeated1,
  HDU_V2_REPEATED_FIELD(f0,all_types::TYPE,1)
)
HDU_V2_UNIT(wire_unit_repeated_protobuf1,
  HDU_V2_REPEATED_FIELD(f0,all_types::TYPE,1)
)

HDU_V2_UNIT(tree,
  HDU_V2_DEFAULT_FIELD(f0,TYPE_INT32,1,5)
  HDU_V2_REPEATED_FIELD(children,TYPE_DATAUNIT,2,false,Auto,Auto,External)
)
