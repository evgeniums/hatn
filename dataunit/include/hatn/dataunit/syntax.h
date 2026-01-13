/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*

*/
/** @file dataunit/syntax.h
  *
  *      Hatn DataUnit top level header to use in sources for DataUnit definitions
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITSYNTAX_H
#define HATNDATAUNITSYNTAX_H

#include <hatn/dataunit/unitmacros.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//! Declare data unit with name UnitName.
//!
/**
 *
 * @example
 *
 * HDU_UNIT(first_unit, // <- only data unit name is followed by comma
 *      HDU_FIELD(field1,TYPE_INT32,1) // <- no commas here
 *      HDU_FIELD_REQUIRED(field2,TYPE_INT8,2)
 *      HDU_FIELD(field3,TYPE_BYTES,3)
 *      HDU_FIELD(field4,TYPE_INT64,4
 * )
 *
 *
 */
#define HDU_UNIT(UnitName,...) HDU_V2_UNIT(UnitName,__VA_ARGS__)

//! Declare data unit that inherits field definitions from other data unit(s).
/**
 * @param UnitName Name of the unit.
 * @param Base List of the base units this unit inherits from. Must be surrounded with parenthesis.
 *
 * @example HDU_UNIT_WITH(unit3,(HDU_BASE(unit1),HDU_BASE(unit2)),...)
 */
#define HDU_UNIT_WITH(UnitName,Base,...) HDU_V2_UNIT_WITH(UnitName,Base,__VA_ARGS__)

//! Wrapper of data unit name to use in inheritance macro HDU_UNIT_WITH
/**
 * @param UnitName Name of the base unit.
 *
 * @example HDU_UNIT_WITH(unit3,(HDU_BASE(unit1),HDU_BASE(unit2)),...)
 */
#define HDU_BASE(UnitName) HDU_V2_BASE(UnitName)

//! Use this macro to declare empty data unit.
#define HDU_UNIT_EMPTY(UnitName) HDU_V2_UNIT_EMPTY(UnitName)

//! Use this macro to declare unit field.
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Integer identificator that must be unique per unit.
 * @param Required true if this is a required field.
 * @param Default Default value for the field. Parameter can be omitted.
 *
 * Full form: HDU_FIELD(FieldName,Type,Id,Required,Default)
 * Optional field: HDU_FIELD(FieldName,Type,Id)
 * Required field: HDU_FIELD(FieldName,Type,Id,true)
 *
 * @example HDU_FIELD(f1,TYPE_UINT32,1)
 * @example HDU_FIELD(f2,TYPE_UINT32,2,true)
 * @example HDU_FIELD(f3,TYPE_UINT32,2,false,100)
 */
#define HDU_FIELD(...) HDU_V2_FIELD(__VA_ARGS__)

//! Use this macro to declare required field.
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Integer identificator that must be unique per unit.
 */
#define HDU_REQUIRED_FIELD(FieldName,Type,Id) HDU_V2_REQUIRED_FIELD(FieldName,Type,Id)

//! Use this macro to declare a default field.
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Integer identificator that must be unique per unit.
 * @param Default Default value for the field. Parameter can be omitted.
 */
#define HDU_DEFAULT_FIELD(FieldName,Type,Id,Default) HDU_V2_DEFAULT_FIELD(FieldName,Type,Id,Default)

//! Use this macro to declare a repeated field.
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Integer identificator that must be unique per unit.
 * @param Required true if this is a required field.
 * @param Default Default value for the elements. Parameter can be omitted or set to Auto.
 * @param Mode Mode of packing. Parameter can be omitted.
 *        Possible values: Auto - use auto mode of packing depending on value type, ProtobufUnpacked - use protobuf ordinary mode, ProtobufPacked - use protobuf packed mode,
 *        Counted - use experimental optimized mode when serialized data contains count of elements in array which is not safe when using in public API.
 *
 * Full form: HDU_REPEATED_FIELD(FieldName,Type,Id,Required,Default,Mode)
 * Optional repeated field: HDU_REPEATED_FIELD(FieldName,Type,Id)
 * Required repeated field: HDU_REPEATED_FIELD(FieldName,Type,Id,true)
 * Repeated field with default element values: HDU_REPEATED_FIELD(FieldName,Type,Id,Required,Default)
 * Repeated field explicit mode: HDU_REPEATED_FIELD(FieldName,Type,Id,Required,Default,Mode)
 *
 * @example HDU_REPEATED_FIELD(rf1,TYPE_INT32,1)
 * @example HDU_REPEATED_FIELD(rf2,TYPE_INT32,2,true)
 * @example HDU_REPEATED_FIELD(rf3,TYPE_INT32,3,false,333)
 * @example HDU_REPEATED_FIELD(rf4,TYPE_INT32,4,true,Auto,ProtobufUnpacked)
 *
 *
 */
#define HDU_REPEATED_FIELD(...) HDU_V2_REPEATED_FIELD(__VA_ARGS__)

//! Use this macro to declare a repeated field of basic type which is not subunit.
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Integer identificator that must be unique per unit.
 * @param Required true if this is a required field.
 * @param Default Default value for the elements.
 */
#define HDU_REPEATED_BASIC_FIELD(FieldName,Type,Id,Required,Default) HDU_V2_REPEATED_BASIC_FIELD(FieldName,Type,Id,Required,Default)

//! Use this macro to declare a repeated subunit field.
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Integer identificator that must be unique per unit.
 * @param Required true if this is a required field.
 */
#define HDU_REPEATED_UNIT_FIELD(FieldName,Type,Id,Required) HDU_V2_REPEATED_UNIT_FIELD(FieldName,Type,Id,Required)

//! Use this macro to declare an enum.
/**
 * @param EnumName Name of enum.
 * @param Values List of value names.
 *
 * Actual integer values will be implicitly bound to names.
 *
 * @example HDU_ENUM(e1,One,Two,Three)
 */
#define HDU_ENUM(EnumName,...) HDU_V2_ENUM(EnumName,__VA_ARGS__)

//! Use this macro for field of enum type.
/**
 *
 * @example HDU_FIELD(fe1,HDU_TYPE_ENUM(e1),1,false,e1::One)
 *
 */
#define HDU_TYPE_ENUM(Type) HDU_V2_TYPE_ENUM(Type)

//! Use this macro for field of fixed string type.
/**
 * @param Length of fixed string. Can be one of: 8,16,20,32,40,64,128,256,512,1024
 *
 * @example HDU_FIELD(fe1,HDU_TYPE_FIXED_STRING(64),1,false,"Hello world!")
 */
#define HDU_TYPE_FIXED_STRING(Length) HDU_V2_TYPE_FIXED_STRING(Length)


//! Instantiate data unit definitions.
/**
 * @param UnitName Name of the unit.
 * @param ExportAttr Export attribute for DLL exporting. Can be omitted.
 *
 * Call this macro in *.cpp file to compile dataunit instantiation that can be either used later within the same project or exported in the library.
 *
 * @example HDU_INSTANTIATE(unit1)
 * @example HDU_INSTANTIATE(unit2, API_EXPORT)
 *
 * */
#define HDU_INSTANTIATE(...) HDU_V2_INSTANTIATE(__VA_ARGS__)

//! Alias to HDU_INSTANTIATE(UnitName,ExportAttr)
#define HDU_EXPORT(UnitName,ExportAttr) HDU_V2_EXPORT(UnitName,ExportAttr)

/**
 ********************** DataUnit declaration examples *********************************

using namespace hatn::dataunit;
using namespace hatn::dataunit::types;

HDU_UNIT(first_unit, // <- only data unit name is followed by comma
  HDU_FIELD(field1,TYPE_INT32,1) // <- no commas here
  HDU_FIELD_REQUIRED(field2,TYPE_INT8,2)
  HDU_FIELD(field3,TYPE_BYTES,3)
  HDU_FIELD(field4,TYPE_INT64,4)
)

HDU_UNIT(other_unit,
  HDU_FIELD(foo,TYPE_INT32,1)
  HDU_FIELD(bar,TYPE_INT32,2)
  HDU_FIELD_REPEATED(repeated_field,TYPE_INT32,3)
  HDU_FIELD(field4,TYPE_BOOL,4)
  HDU_FIELD(field5,TYPE_DATAUNIT,5)
  HDU_FIELD(unit_field,first_unit::TYPE,6)
)

HDU_DATAUNIT_EMPTY(empty_unit)

 ***************************************************************************************
**/

#define HDU_MAP(Name,Key,Value) \
    HDU_UNIT(Name, \
        HDU_FIELD(key,Key,MapItemKey::id) \
        HDU_FIELD(value,Value,MapItemValue::id) \
    )

#define HDU_MAP_FIELD(...) \
    HDU_V2_MAP_FIELD(__VA_ARGS__)

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSYNTAX_H
