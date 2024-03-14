/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*

*/
/** \file dataunit/syntax.h
  *
  *      Dracosha DataUnit top level header to use in sources for DataUnit definitions
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITSYNTAX_H
#define HATNDATAUNITSYNTAX_H

#include <vector>

#include <hatn/common/logger.h>
#include <hatn/dataunit/dataunit.h>

DECLARE_LOG_MODULE_EXPORT(dataunit,HATN_DATAUNIT_EXPORT)

#include <hatn/dataunit/macros.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//! Use this macro to declare DataUnit with name UnitName
#define HDU_DATAUNIT(UnitName,...) _HDU_DATAUNIT(UnitName,__VA_ARGS__)

//! Use this macro to extend DataUnit with new fields
/**
  * All successive extensions of the same DataUnit will be appended so that the last extension
  * will consists of all preceding extensions and the DataUnit itself.
  *
  */
#define HDU_EXTEND(UnitName,ExtensionName,...) _HDU_EXTEND(UnitName,ExtensionName,__VA_ARGS__)

//! Use this macro to declare DataUnit with name UnitName without fields
#define HDU_DATAUNIT_EMPTY(UnitName) _HDU_DATAUNIT_EMPTY(UnitName)

//! Use this macro to declare an optional dataunit field with name FieldName
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD(...) _HDU_FIELD_DEF(__VA_ARGS__)

//! Use this macro to declare a required dataunit field
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_REQUIRED(...) _HDU_FIELD_REQUIRED_DEF(__VA_ARGS__)

//! Use this macro to declare a required dataunit field
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_DEFAULT(...) _HDU_FIELD_DEFAULT_DEF(__VA_ARGS__)

//! Use this macro to declare a dataunit field that is of other unit
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 *
 * The field can be used either as embedded field with traits::type or as shared field with traits::shared_fields_type
 */
#define HDU_FIELD_DATAUNIT(...) _HDU_FIELD_DATAUNIT_DEF(__VA_ARGS__)

//! Use this macro to declare an external linked dataunit field
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_EXTERNAL(...) _HDU_FIELD_EXTERNAL_DEF(__VA_ARGS__)

//! Use this macro to declare an embedded dataunit field with shared embedded comand
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_EMBEDDED(...) _HDU_FIELD_EMBEDDED_DEF(__VA_ARGS__)

//! Use this macro to declare a repeated dataunit field with strong type check
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_REPEATED(...) _HDU_FIELD_REPEATED_DEF(__VA_ARGS__)

//! Use this macro to declare a repeated dataunit field with default value with strong type check
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_REPEATED_DEFAULT(...) _HDU_FIELD_REPEATED_DEFAULT_DEF(__VA_ARGS__)

//! Use this macro to declare a repeated DataUnit containig other units
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_REPEATED_DATAUNIT(...) _HDU_FIELD_REPEATED_DATAUNIT_DEF(__VA_ARGS__)

//! Use this macro to declare a repeated dataunit field on shared data units
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_REPEATED_EXTERNAL(...) _HDU_FIELD_REPEATED_EXTERNAL_DEF(__VA_ARGS__)

//! Use this macro to declare a repeated dataunit field on embedded data units
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_REPEATED_EMBEDDED(...) _HDU_FIELD_REPEATED_EMBEDDED_DEF(__VA_ARGS__)

//! Use this macro to reserve field ID
#define HDU_FIELD_RESERVE_ID(Id) _HDU_FIELD_RESERVE_ID(Id)

//! Use this macro to reserve field name
#define HDU_FIELD_RESERVE_NAME(FieldName) _HDU_FIELD_RESERVE_NAME(FieldName)

//! Use this macro to declare an enum
#define HDU_ENUM(EnumName,...) _HDU_ENUM(EnumName,__VA_ARGS__)

//! Use this macro for enum type
#define HDU_TYPE_ENUM(Type) _HDU_TYPE_ENUM(Type)

//! Use this macro for fixed string type
#define HDU_TYPE_FIXED_STRING(Length) _HDU_TYPE_FIXED_STRING(Length)

//! Use this macro to declare a repeated dataunit field compatible with repeated unpacked type of Google Protocol Buffers for ordinary types
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_REPEATED_PROTOBUF_ORDINARY(...) _HDU_FIELD_REPEATED_PROTOBUF_ORDINARY_DEF(__VA_ARGS__)

//! Use this macro to declare a repeated dataunit field compatible with repeated packed type of Google Protocol Buffers
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_REPEATED_PROTOBUF_PACKED(...) _HDU_FIELD_REPEATED_PROTOBUF_PACKED_DEF(__VA_ARGS__)

//! Use this macro to declare a repeated DataUnit containig other units compatible with repeated unpacked type of Google Protocol Buffers
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 *
 * The field can be used either as embedded field with traits::type or as shared field with traits::shared_fields_type
 */
#define HDU_FIELD_REPEATED_DATAUNIT_PROTOBUF(...) _HDU_FIELD_REPEATED_DATAUNIT_PROTOBUF_DEF(__VA_ARGS__)

//! Use this macro to declare a repeated external dataunit field compatible with repeated unpacked type of Google Protocol Buffers
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_REPEATED_EXTERNAL_PROTOBUF(...) _HDU_FIELD_REPEATED_EXTERNAL_PROTOBUF_DEF(__VA_ARGS__)

//! Use this macro to declare a repeated embedded dataunit field compatible with repeated unpacked type of Google Protocol Buffers
/**
 * @param FieldName Name of the field.
 * @param Type Field type, Type must be one of supported types.
 * @param Id Identificator which is an integer that is unique per each field in the unit
 * @param Description Optional field description, can be ommited.
 */
#define HDU_FIELD_REPEATED_EMBEDDED_PROTOBUF(...) _HDU_FIELD_REPEATED_EMBEDDED_PROTOBUF_DEF(__VA_ARGS__)

//! Instantiate DataUnit class
#define HDU_INSTANTIATE_DATAUNIT(UnitName) _HDU_INSTANTIATE_DATAUNIT(UnitName)

//! Instantiate DataUnit extntion class
#define HDU_INSTANTIATE_DATAUNIT_EXT(UnitName,ExtentionName) _HDU_INSTANTIATE_DATAUNIT_EXT(UnitName,ExtentionName)

/**
 ********************** DataUnit declaration examples *********************************

using namespace hatn::dataunit;

HDU_DATAUNIT(first_unit, <- only data unit name is followed by comma
  HDU_FIELD(field1,TYPE_INT32,1) <- no commas here
  HDU_FIELD_REQUIRED(field2,TYPE_INT8,2)
  HDU_FIELD(field3,TYPE_BYTES,3)
  HDU_FIELD(field4,TYPE_INT64,4)
)

HDU_DATAUNIT(other_unit,
  HDU_FIELD(foo,TYPE_INT32,1)
  HDU_FIELD(bar,TYPE_INT32,2)
  HDU_FIELD_REPEATED(repeated_field,TYPE_INT32,3)
  HDU_FIELD(field4,TYPE_BOOL,4)
  HDU_FIELD(field5,TYPE_DATAUNIT,5)
  HDU_FIELD_DATAUNIT(unit_field,first_unit::TYPE,6)
)

HDU_DATAUNIT_EMPTY(empty_unit)

 ***************************************************************************************
**/

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSYNTAX_H
