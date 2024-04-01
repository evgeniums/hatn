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

#define HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,default_traits,required,repeated_traits) \
    struct field_##FieldName{};\
    template <>\
    struct field<HATN_COUNTER_GET(c)>\
    {\
        using hana_tag=field_tag;\
        using id=field_##FieldName;\
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
    constexpr field<HATN_COUNTER_GET(c)> FieldName{}; \
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
    HDU_V2_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,false)

#define HDU_V2_REQUIRED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
    HDU_V2_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,true)

#define HDU_V2_DEFAULT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Default) \
    HDU_V2_DEFAULT_TRAITS(FieldName,Type,Default)\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,default_##FieldName,hana::false_,hana::false_)

#define HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Mode,Required,Default) \
    HDU_V2_DEFAULT_TRAITS(FieldName,Type,Default)\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,default_##FieldName,Required,Mode)

#define HDU_V2_REPEATED_FIELD_NORMAL_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required,Default) \
    using mode_##FieldName=std::integral_constant<RepeatedMode,RepeatedMode::Normal>;\
    HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,mode_##FieldName,hana::bool_<Required>,Default)

#define HDU_V2_REPEATED_FIELD_PBPACKED_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required,Default) \
    using mode_##FieldName=std::integral_constant<RepeatedMode,RepeatedMode::ProtobufPacked>;\
    HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,mode_##FieldName,hana::bool_<Required>,Default)

#define HDU_V2_REPEATED_FIELD_PBORDINARY_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required,Default) \
    using mode_##FieldName=std::integral_constant<RepeatedMode,RepeatedMode::ProtobufOrdinary>;\
    HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,mode_##FieldName,hana::bool_<Required>,Default)

#define HDU_V2_DATAUNIT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    HDU_V2_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required)

#define HDU_V2_DATAUNIT(UnitName,...) \
    namespace UnitName { \
        using namespace hatn::dataunit; \
        using namespace hatn::dataunit::types; \
        struct conf{constexpr static const char* name=#UnitName;};\
        struct c{};\
        HATN_COUNTER_MAKE(c);\
        template <int N> struct field{};\
        __VA_ARGS__ \
        auto fields=make_fields_tuple<field,HATN_COUNTER_GET(c)>();\
        auto unit_c=unit<conf>::type_c(fields);\
        auto shared_unit_c=unit<conf>::shared_type_c(fields);\
        using type=decltype(unit_c)::type;\
        using shared_type=decltype(shared_unit_c)::type;\
        using managed=managed_unit<type>;\
        using shared_managed=shared_managed_unit<shared_type>;\
        using traits=unit_traits<type,managed,decltype(fields)>;\
        using shared_traits=unit_traits<shared_type,shared_managed,decltype(fields)>;\
        using TYPE=subunit<traits,shared_traits>;\
}

#endif // HATNDATAUNITMACROS_H
