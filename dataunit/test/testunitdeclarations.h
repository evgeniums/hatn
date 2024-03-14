#ifdef _WIN32
#define HDU_DATAUNIT_EXPORT __declspec(dllexport)
#else
#define HDU_DATAUNIT_EXPORT
#endif

#include <hatn/common/pmr/withstaticallocator.h>
#define HATN_WITH_STATIC_ALLOCATOR_SRC
#ifdef HATN_WITH_STATIC_ALLOCATOR_SRC
    #include <hatn/common/pmr/withstaticallocator.ipp>
    #define HATN_WITH_STATIC_ALLOCATOR_INLINE HATN_WITH_STATIC_ALLOCATOR_INLINE_SRC
#else
    #define HATN_WITH_STATIC_ALLOCATOR_INLINE HATN_WITH_STATIC_ALLOCATOR_INLINE_H
#endif

#include <hatn/dataunit/syntax.h>

HDU_DATAUNIT(simple_int8,
  HDU_FIELD(type_int8,TYPE_INT8,2)
)

HDU_DATAUNIT(names_and_descr,
    HDU_FIELD(optional_field,TYPE_INT8,1)
    HDU_FIELD(optional_field_descr,TYPE_INT8,2,"Optional field with description")

    HDU_FIELD_REQUIRED(required_field,TYPE_INT8,3)
    HDU_FIELD_REQUIRED(required_field_descr,TYPE_INT8,4,"Required field with description")

    HDU_FIELD_DEFAULT(default_field,TYPE_INT8,5,1)
    HDU_FIELD_DEFAULT(default_field_descr,TYPE_INT8,6,1,"Default field with description")

    HDU_FIELD_REPEATED(repeated_field,TYPE_INT8,7)
    HDU_FIELD_REPEATED(repeated_field_descr,TYPE_INT8,8,"Repeated field with description")
)

HDU_DATAUNIT(subunit_names_and_descr,
    HDU_FIELD_DATAUNIT(dataunit_field,names_and_descr::TYPE,1)
    HDU_FIELD_DATAUNIT(dataunit_field_descr,names_and_descr::TYPE,2,"Dataunit field with description")
    HDU_FIELD_DATAUNIT(external_field,names_and_descr::TYPE,3)
    HDU_FIELD_DATAUNIT(external_field_descr,names_and_descr::TYPE,4,"External field with description")
    HDU_FIELD_DATAUNIT(embedded_field,names_and_descr::TYPE,5)
    HDU_FIELD_DATAUNIT(embedded_field_descr,names_and_descr::TYPE,6,"Embedded field with description")
)

#if 1

HDU_DATAUNIT(all_types,
  HDU_FIELD(type_bool,TYPE_BOOL,1)
  HDU_FIELD(type_int8,TYPE_INT8,2)
  HDU_FIELD(type_int16,TYPE_INT16,3)
  HDU_FIELD(type_int32,TYPE_INT32,4)
  HDU_FIELD(type_int64,TYPE_INT64,5)
  HDU_FIELD(type_uint8,TYPE_UINT8,6)
  HDU_FIELD(type_uint16,TYPE_UINT16,7)
  HDU_FIELD(type_uint32,TYPE_UINT32,8)
  HDU_FIELD(type_uint64,TYPE_UINT64,9)
  HDU_FIELD(type_float,TYPE_FLOAT,10,"Field with description")
  HDU_FIELD(type_double,TYPE_DOUBLE,11)
  HDU_FIELD(type_string,TYPE_STRING,12)
  HDU_FIELD(type_bytes,TYPE_BYTES,13)
  HDU_FIELD(type_fixed_string,HDU_TYPE_FIXED_STRING(8),20)
  HDU_FIELD_REQUIRED(type_int8_required,TYPE_INT8,25)
  HDU_ENUM(MyEnum,One=1,Two=2)
)

HDU_DATAUNIT(repeated,
  HDU_FIELD_REPEATED(type_repeated,TYPE_INT32,21)
  HDU_FIELD_REPEATED_PROTOBUF_ORDINARY(type_repeated_pbuf,TYPE_INT32,22)
  HDU_FIELD_REPEATED_PROTOBUF_PACKED(type_repeated_pbuf_packed,TYPE_INT32,23)
  HDU_FIELD_REPEATED(type_repeated_fixed_str1,HDU_TYPE_FIXED_STRING(8),24)
)

HDU_EXTEND(all_types,ext0,
    HDU_FIELD(ext0_int32,TYPE_INT32,202)
)
HDU_EXTEND(all_types,ext1,
    HDU_FIELD(type_bool1,TYPE_BOOL,100)
)

HDU_DATAUNIT(all_types_copy,
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
   HDU_FIELD_REQUIRED(type_int8_required,TYPE_INT8,25)
)

HDU_DATAUNIT_EMPTY(empty_unit)

HDU_EXTEND(empty_unit,ext2,
    HDU_FIELD(type_int8,TYPE_INT8,2)
)

HDU_DATAUNIT(embedded_unit,
  HDU_FIELD_EMBEDDED(f0,all_types::TYPE,1)
)
HDU_DATAUNIT(shared_unit,
  HDU_FIELD_EXTERNAL(f0,all_types::TYPE,1)
)

HDU_DATAUNIT(with_unit,
  HDU_FIELD_DATAUNIT(f0,all_types::TYPE,1)
)

HDU_DATAUNIT(with_unit2,
  HDU_FIELD_DATAUNIT(f0,simple_int8::TYPE,1)
)
HDU_DATAUNIT(with_unit3,
  HDU_FIELD_DATAUNIT(f0,with_unit2::TYPE,1)
)
HDU_DATAUNIT(with_unit4,
  HDU_FIELD_DATAUNIT(f0,with_unit3::TYPE,1)
  HDU_FIELD_DATAUNIT(f1,with_unit2::TYPE,2)
)

HDU_DATAUNIT(default_fields,
  HDU_FIELD_DEFAULT(type_bool,TYPE_BOOL,1,true)
  HDU_FIELD_DEFAULT(type_int32,TYPE_INT32,2,1000)
  HDU_FIELD_DEFAULT(type_float,TYPE_FLOAT,3,1010.2150f)
  HDU_FIELD_DEFAULT(type_double,TYPE_DOUBLE,4,3020.1123f)
  HDU_ENUM(MyEnum,One=1,Two=2)
  HDU_FIELD_DEFAULT(type_enum,HDU_TYPE_ENUM(MyEnum),100,MyEnum::Two)
  HDU_FIELD_REPEATED_DEFAULT(type_double_repeated,TYPE_DOUBLE,5,3030.3f)
  HDU_FIELD_REPEATED_DEFAULT(type_enum_repeated,HDU_TYPE_ENUM(MyEnum),101,MyEnum::One)
)


HDU_DATAUNIT(wire_bool,
  HDU_FIELD(f0,TYPE_BOOL,1)
)
HDU_DATAUNIT(wire_uint8,
  HDU_FIELD(f0,TYPE_UINT8,1)
)
HDU_DATAUNIT(wire_uint16,
  HDU_FIELD(f0,TYPE_UINT16,1)
)
HDU_DATAUNIT(wire_uint32,
  HDU_FIELD(f0,TYPE_UINT32,1)
)
HDU_DATAUNIT(wire_uint64,
  HDU_FIELD(f0,TYPE_UINT64,1)
)
HDU_DATAUNIT(wire_int8,
  HDU_FIELD(f0,TYPE_INT8,200)
)
HDU_DATAUNIT(wire_int16,
  HDU_FIELD(f0,TYPE_INT16,10)
)
HDU_DATAUNIT(wire_int32,
  HDU_FIELD(f0,TYPE_INT32,3)
)
HDU_DATAUNIT(wire_int64,
  HDU_FIELD(f0,TYPE_INT64,2)
)

HDU_DATAUNIT(wire_fixed_uint32,
  HDU_FIELD(f0,TYPE_FIXED_UINT32,1)
)
HDU_DATAUNIT(wire_fixed_uint64,
  HDU_FIELD(f0,TYPE_FIXED_UINT64,1)
)
HDU_DATAUNIT(wire_fixed_int32,
  HDU_FIELD(f0,TYPE_FIXED_INT32,1)
)
HDU_DATAUNIT(wire_fixed_int64,
  HDU_FIELD(f0,TYPE_FIXED_INT64,1)
)

HDU_DATAUNIT(wire_float,
  HDU_FIELD(f0,TYPE_FLOAT,1)
)
HDU_DATAUNIT(wire_double,
  HDU_FIELD(f0,TYPE_DOUBLE,1)
)

HDU_DATAUNIT(wire_bytes,
  HDU_FIELD(f0,TYPE_BYTES,1)
)

HDU_DATAUNIT(wire_uint8_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_UINT8,1)
)
HDU_DATAUNIT(wire_uint16_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_UINT16,1)
)
HDU_DATAUNIT(wire_uint32_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_UINT32,1)
)
HDU_DATAUNIT(wire_uint64_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_UINT64,1)
)
HDU_DATAUNIT(wire_int8_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_INT8,200)
)
HDU_DATAUNIT(wire_int16_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_INT16,10)
)
HDU_DATAUNIT(wire_int32_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_INT32,3)
)
HDU_DATAUNIT(wire_int64_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_INT64,2)
)

HDU_DATAUNIT(wire_fixed_uint32_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_FIXED_UINT32,1)
)
HDU_DATAUNIT(wire_fixed_uint64_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_FIXED_UINT64,1)
)
HDU_DATAUNIT(wire_fixed_int32_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_FIXED_INT32,1)
)
HDU_DATAUNIT(wire_fixed_int64_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_FIXED_INT64,1)
)

HDU_DATAUNIT(wire_float_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_FLOAT,1)
)

HDU_DATAUNIT(wire_double_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_DOUBLE,1)
)
HDU_DATAUNIT(wire_double_repeated_proto_packed,
  HDU_FIELD_REPEATED_PROTOBUF_PACKED(f0,TYPE_DOUBLE,1)
)
HDU_DATAUNIT(wire_double_repeated_proto,
  HDU_FIELD_REPEATED_PROTOBUF_ORDINARY(f0,TYPE_DOUBLE,1)
)

HDU_DATAUNIT(wire_bytes_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_BYTES,1)
)
HDU_DATAUNIT(wire_bytes_repeated_proto,
  HDU_FIELD_REPEATED_PROTOBUF_ORDINARY(f0,TYPE_BYTES,1)
)
HDU_DATAUNIT(wire_string_repeated,
  HDU_FIELD_REPEATED(f0,TYPE_STRING,1)
)
HDU_DATAUNIT(wire_string_repeated_proto,
  HDU_FIELD_REPEATED_PROTOBUF_ORDINARY(f0,TYPE_STRING,1)
)
HDU_DATAUNIT(wire_fixed_string_repeated,
  HDU_FIELD_REPEATED(f0,HDU_TYPE_FIXED_STRING(128),1)
)
HDU_DATAUNIT(wire_fixed_string_repeated_proto,
  HDU_FIELD_REPEATED_PROTOBUF_ORDINARY(f0,HDU_TYPE_FIXED_STRING(128),1)
)

HDU_DATAUNIT(wire_unit_repeated,
  HDU_FIELD_REPEATED_EXTERNAL(f0,all_types::TYPE,1)
)

HDU_DATAUNIT(wire_embedded_repeated,
  HDU_FIELD_REPEATED_EMBEDDED(f0,empty_unit::TYPE,1)
)

HDU_DATAUNIT(wire_embedded_uint8_repeated,
  HDU_FIELD_REPEATED_EMBEDDED(f0,wire_uint8::TYPE,1)
)

HDU_DATAUNIT(wire_embedded_all_repeated,
  HDU_FIELD_REPEATED_EMBEDDED(f0,all_types::TYPE,1)
)

HDU_DATAUNIT(wire_embedded_all_repeated_protobuf,
  HDU_FIELD_REPEATED_EMBEDDED_PROTOBUF(f0,all_types::TYPE,1)
)
HDU_DATAUNIT(wire_external_all_repeated_protobuf,
  HDU_FIELD_REPEATED_EXTERNAL_PROTOBUF(f0,all_types::TYPE,1)
)

HDU_DATAUNIT(wire_simple_unit_repeated,
  HDU_FIELD_REPEATED_EXTERNAL(f0,simple_int8::TYPE,1)
)

HDU_DATAUNIT(wire_unit_repeated1,
  HDU_FIELD_REPEATED_DATAUNIT(f0,all_types::TYPE,1)
)
HDU_DATAUNIT(wire_unit_repeated_protobuf1,
  HDU_FIELD_REPEATED_DATAUNIT(f0,all_types::TYPE,1)
)

HDU_DATAUNIT(tree,
  HDU_FIELD_DEFAULT(f0,TYPE_INT32,1,5)
  HDU_FIELD_REPEATED_EXTERNAL(children,TYPE_DATAUNIT,2)
)

#if 0

HDU_DATAUNIT(many_fields,
  HDU_FIELD(b1,TYPE_BOOL,1)
  HDU_FIELD(id,TYPE_INT32,4)
  HDU_FIELD(time,TYPE_INT64,5)
  HDU_FIELD(name,TYPE_STRING,12)
  HDU_FIELD(lastname,TYPE_STRING,13)
  HDU_FIELD(middlename,TYPE_STRING,14)
  HDU_FIELD(position,TYPE_STRING,15)
  HDU_FIELD(country,TYPE_STRING,16)
  HDU_FIELD(city,TYPE_STRING,17)
  HDU_FIELD(streetaddress,TYPE_STRING,18)
  HDU_FIELD(department,TYPE_STRING,19)
  HDU_FIELD(phone,TYPE_STRING,20)
  HDU_FIELD(email,TYPE_STRING,21)
  HDU_FIELD(login,TYPE_STRING,22)
  HDU_FIELD(image,TYPE_BYTES,23)
  HDU_FIELD(inn,TYPE_BYTES,24)
  HDU_FIELD(organization,TYPE_STRING,25)
  HDU_FIELD_REPEATED(groups,TYPE_STRING,26)
  HDU_FIELD_REPEATED(friends,TYPE_STRING,27)
  HDU_FIELD_REPEATED(hobbies,TYPE_STRING,28)
  HDU_FIELD_REPEATED(tags,TYPE_STRING,29)
)
#endif
#endif
#undef HATN_WITH_STATIC_ALLOCATOR_INLINE
