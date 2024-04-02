/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file dataunit/unitmacros.h
  *
  *  Contains macros for data unit declaration.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITMACROS_H
#define HATNDATAUNITMACROS_H

#include <hatn/dataunit/unitmeta.h>

#define HDU_V2_IS_BASIC_TYPE(FieldName,Type) \
static_assert(decltype(meta::is_basic_type<Type>())::value,"Invalid field type for "#FieldName);

#define HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
static_assert(decltype(meta::is_unit_type<Type>())::value,"Only dataunit types can be used in this expression for "#FieldName);

#define HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,default_traits,required,repeated_traits) \
    struct field_##FieldName{};\
    template <>\
    struct field<HATN_COUNTER_GET(c)>\
    {\
        struct strings\
        {\
                constexpr static const char* name=#FieldName;\
                constexpr static const char* description=#Description;\
        };\
        using traits=field_traits<field_generator,\
                       strings,\
                       hana::int_<Id>,\
                       hana::int_<HATN_COUNTER_GET(c)>,\
                       Type,\
                       default_traits,\
                       required,\
                       repeated_traits\
                       >;\
        using type=typename traits::type;\
        using shared_type=typename traits::shared_type;\
    };\
    constexpr typename field<HATN_COUNTER_GET(c)>::traits FieldName{};\
    constexpr typename field<HATN_COUNTER_GET(c)>::traits inst_##FieldName{};\
    template <> struct field_id<HATN_COUNTER_GET(c)+1> : public field_id<HATN_COUNTER_GET(c)>\
    {\
        constexpr static const auto& FieldName=inst_##FieldName;\
    };\
    HATN_COUNTER_INC(c);

#define HDU_V2_DEFAULT_TRAITS(FieldName,Type,Default) \
struct default_##FieldName\
{\
        using hana_tag=DefaultValueTag; \
        static typename Type::type value(){return static_cast<typename Type::type>(Default);} \
        using HasDefV=std::true_type; \
};

#define HDU_V2_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,hana::false_)

#define HDU_V2_OPTIONAL_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
    HDU_V2_IS_BASIC_TYPE(FieldName,Type) \
    HDU_V2_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,false)

#define HDU_V2_REQUIRED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
    HDU_V2_IS_BASIC_TYPE(FieldName,Type) \
    HDU_V2_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,true)

#define HDU_V2_DEFAULT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Default) \
    HDU_V2_IS_BASIC_TYPE(FieldName,Type) \
    HDU_V2_DEFAULT_TRAITS(FieldName,Type,Default)\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,default_##FieldName,hana::false_,hana::false_)

#define HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,RepeatedConfig,Required,Default) \
    HDU_V2_IS_BASIC_TYPE(FieldName,Type) \
    HDU_V2_DEFAULT_TRAITS(FieldName,Type,Default)\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,default_##FieldName,Required,RepeatedConfig)

#define HDU_V2_REPEATED_FIELD_NORMAL_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required,Default) \
    HDU_V2_IS_BASIC_TYPE(FieldName,Type) \
    using cfg_##FieldName=repeated_config<RepeatedMode::Normal>;\
    HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,cfg_##FieldName,hana::bool_<Required>,Default)

#define HDU_V2_REPEATED_FIELD_PBPACKED_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required,Default) \
    HDU_V2_IS_BASIC_TYPE(FieldName,Type) \
    using cfg_##FieldName=repeated_config<RepeatedMode::ProtobufPacked>;\
    HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,cfg_##FieldName,hana::bool_<Required>,Default)

#define HDU_V2_REPEATED_FIELD_PBORDINARY_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required,Default) \
    HDU_V2_IS_BASIC_TYPE(FieldName,Type) \
    using cfg_##FieldName=repeated_config<RepeatedMode::ProtobufOrdinary>;\
    HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,cfg_##FieldName,hana::bool_<Required>,Default)

#define HDU_V2_UNIT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    HDU_V2_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required)

#define HDU_V2_REPEATED_UNIT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    using cfg_##FieldName=repeated_config<RepeatedMode::Normal,RepeatedContentType::AutoDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_REPEATED_EXTERNAL_UNIT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    using cfg_##FieldName=repeated_config<RepeatedMode::Normal,RepeatedContentType::ExternalDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_REPEATED_EMBEDDED_UNIT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    using cfg_##FieldName=repeated_config<RepeatedMode::Normal,RepeatedContentType::EmbeddedDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_REPEATED_UNIT_FIELD_PBORDINARY_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    using cfg_##FieldName=repeated_config<RepeatedMode::ProtobufOrdinary,RepeatedContentType::AutoDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_REPEATED_EXTERNAL_UNIT_FIELD_PBORDINARY_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    using cfg_##FieldName=repeated_config<RepeatedMode::ProtobufOrdinary,RepeatedContentType::ExternalDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_REPEATED_EMBEDDED_UNIT_FIELD_PBORDINARY_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    using cfg_##FieldName=repeated_config<RepeatedMode::ProtobufOrdinary,RepeatedContentType::EmbeddedDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_ENUM(EnumName,...) \
enum class EnumName : int {__VA_ARGS__};

#define HDU_V2_TYPE_ENUM(Type) TYPE_ENUM<Type>

#define HDU_V2_TYPE_FIXED_STRING(Length) \
    TYPE_FIXED_STRING<Length>

#define HDU_V2_UNIT_BEGIN(UnitName) \
    namespace UnitName { \
    auto base_fields=boost::hana::make_tuple();

#define HDU_V2_UNIT_BODY(UnitName,...) \
    using namespace HATN_DATAUNIT_NAMESPACE; \
    using namespace HATN_DATAUNIT_NAMESPACE::types; \
    using namespace HATN_DATAUNIT_META_NAMESPACE; \
    struct conf{constexpr static const char* name=#UnitName;};\
    struct c{};\
    HATN_COUNTER_MAKE(c);\
    template <int N> struct field{};\
    template <int N> struct field_id{};\
    template <> struct field_id<0>{using hana_tag=field_id_tag;}; \
    __VA_ARGS__ \
    auto field_defs=boost::hana::concat(base_fields,make_fields_tuple<field,HATN_COUNTER_GET(c)>());\
    auto unit_c=unit<conf>::type_c(field_defs);\
    auto shared_unit_c=unit<conf>::shared_type_c(field_defs);\
    using fields_t=field_id<HATN_COUNTER_GET(c)>;\
    using type=unit_t<decltype(unit_c)::type>;\
    using shared_type=unit_t<decltype(shared_unit_c)::type>;\
    using managed=managed_unit<type>;\
    using shared_managed=shared_managed_unit<shared_type>;\
    /* types below are explicitly derived instead of just "using" in order to decrease object code size */ \
    struct fields : public fields_t{};\
    struct traits : public unit_traits<type,managed,fields>{};\
    struct shared_traits : public unit_traits<shared_type,shared_managed,fields>{};\
    struct TYPE : public subunit<traits,shared_traits>{}; \
    }

#define HDU_V2_UNIT(UnitName,...) \
    HDU_V2_UNIT_BEGIN(UnitName) \
    HDU_V2_UNIT_BODY(UnitName,__VA_ARGS__)

#define HDU_V2_BASE(UnitName) UnitName::field_defs

#define HDU_V2_UNIT_WITH_BEGIN(UnitName, Base) \
    namespace UnitName { \
    auto base_fields=HATN_DATAUNIT_META_NAMESPACE::concat_fields Base;

#define HDU_V2_UNIT_WITH(UnitName, Base, ...) \
    HDU_V2_UNIT_WITH_BEGIN(UnitName, Base) \
    HDU_V2_UNIT_BODY(UnitName,__VA_ARGS__)

#endif // HATNDATAUNITMACROS_H
