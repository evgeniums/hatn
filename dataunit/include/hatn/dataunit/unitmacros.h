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

#include <hatn/common/meta/compilecounter.h>

#include <hatn/dataunit/unitmeta.h>

//---------------------------------------------------------------

#ifdef HATN_STRING_LITERAL
#define HDU_V2_FIELD_NAME_STR(FieldName) constexpr static auto name=#FieldName""_s;
#else
#define HDU_V2_FIELD_NAME_STR(FieldName) constexpr static auto name=#FieldName;
#endif

//---------------------------------------------------------------

#define HDU_V2_IS_BASIC_TYPE(FieldName,Type,Repeated) \
static_assert(decltype(meta::is_basic_type<Type>())::value || hana::is_a<repeated_tag,decltype(Repeated)> ,"Invalid field type for "#FieldName);

#define HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    static_assert(decltype(meta::is_unit_type<Type>())::value,"Only dataunit types can be used in this expression for "#FieldName);

#define HDU_V2_IS_FIELD_TYPE(FieldName,Type) \
static_assert(decltype(meta::is_unit_type<Type>())::value || decltype(meta::is_basic_type<Type>())::value || decltype(meta::is_custom_type<Type>())::value,"Invalid field type for "#FieldName);

#define HDU_V2_CHECK_ID(FieldName,Id) \
    static_assert(std::is_integral<decltype(Id)>::value && Id>0,"ID must be positive integer for "#FieldName);

//---------------------------------------------------------------

#define HDU_V2_DEFAULT_PREPARE(FieldName,Type,Default) \
    constexpr static auto dv_##FieldName=Default;\
    struct default_##FieldName\
    {\
        constexpr static auto& value=dv_##FieldName;\
        using type=std::decay_t<decltype(value)>;\
    };

//---------------------------------------------------------------

#define HDU_V2_FIELD_DEF(FieldName,Type,Id,required,Default,repeated_traits) \
    HDU_V2_CHECK_ID(FieldName,Id) \
    HDU_V2_DEFAULT_PREPARE(FieldName,Type,Default) \
    struct field_##FieldName{};\
    template <>\
    struct field<HATN_COUNTER_GET(c)>\
    {\
        struct strings\
        {\
            constexpr static const char* name=#FieldName;\
        };\
        using traits=field_traits<field_generator,\
                       strings,\
                       hana::int_<Id>,\
                       Type,\
                       default_type<Type,default_##FieldName>,\
                       required,\
                       decltype(repeated_traits)\
                       >;\
        using type=typename traits::type;\
        HDU_V2_FIELD_NAME_STR(FieldName)\
        constexpr static auto id=hana::int_c<Id>;\
    };\
    constexpr typename field<HATN_COUNTER_GET(c)>::traits FieldName{};\
    constexpr typename field<HATN_COUNTER_GET(c)>::traits inst_##FieldName{};\
    template <> struct field_id_<HATN_COUNTER_GET(c)+1> : public field_id_<HATN_COUNTER_GET(c)>\
    {\
        constexpr static const auto& FieldName=inst_##FieldName;\
    };\
    HATN_COUNTER_INC(c);

//---------------------------------------------------------------

#define HDU_V2_EXPAND(x) x
#define HDU_V2_GET_ARG6(arg1, arg2, arg3, arg4, arg5, arg6, ...) arg6
#define HDU_V2_GET_ARG7(arg1, arg2, arg3, arg4, arg5, arg6, arg7, ...) arg7

//---------------------------------------------------------------

#define HDU_V2_FIELD_DEF_OPTIONAL(FieldName,Type,Id,Repeated) \
    HDU_V2_IS_FIELD_TYPE(FieldName,Type) \
    HDU_V2_FIELD_DEF(FieldName,Type,Id,hana::bool_<false>,Auto,Repeated)

#define HDU_V2_OPTIONAL_FIELD(FieldName,Type,Id) HDU_V2_FIELD_DEF_OPTIONAL(FieldName,Type,Id,Auto)

#define HDU_V2_FIELD_DEF_REQUIRED(FieldName,Type,Id,Required,Repeated) \
    HDU_V2_IS_FIELD_TYPE(FieldName,Type) \
    HDU_V2_FIELD_DEF(FieldName,Type,Id,hana::bool_<Required>,Auto,Repeated)

#define HDU_V2_REQUIRED_FIELD(FieldName,Type,Id) HDU_V2_FIELD_DEF_REQUIRED(FieldName,Type,Id,true,Auto)

#define HDU_V2_FIELD_DEF_DEFAULT(FieldName,Type,Id,Required,Default,Repeated) \
    HDU_V2_IS_BASIC_TYPE(FieldName,Type,Repeated) \
    HDU_V2_FIELD_DEF(FieldName,Type,Id,hana::bool_<Required>,Default,Repeated)

#define HDU_V2_DEFAULT_FIELD(FieldName,Type,Id,Default) HDU_V2_FIELD_DEF_DEFAULT(FieldName,Type,Id,false,Default,Auto)

#define HDU_V2_VARG_SELECT_FIELD(...) \
    HDU_V2_EXPAND(HDU_V2_GET_ARG6(__VA_ARGS__, \
                                    HDU_V2_FIELD_DEF_DEFAULT, \
                                    HDU_V2_FIELD_DEF_REQUIRED, \
                                    HDU_V2_FIELD_DEF_OPTIONAL \
                                  ))

#define HDU_V2_FIELD(...) HDU_V2_EXPAND(HDU_V2_VARG_SELECT_FIELD(__VA_ARGS__)(__VA_ARGS__,Auto))

//---------------------------------------------------------------

#define HDU_V2_FIELD_DEF_REPEATED(Mode,FieldName,...) \
    constexpr repeated_config<Mode> cfg_##FieldName{};\
    HDU_V2_EXPAND(HDU_V2_VARG_SELECT_FIELD(FieldName,__VA_ARGS__)(FieldName,__VA_ARGS__,cfg_##FieldName))

#define HDU_V2_REPEATED_FIELD_DEF3(FieldName,Type,Id) \
    HDU_V2_FIELD_DEF_REPEATED(RepeatedMode::Auto,FieldName,Type,Id)

#define HDU_V2_REPEATED_FIELD_DEF4(FieldName,Type,Id,Required) \
    HDU_V2_FIELD_DEF_REPEATED(RepeatedMode::Auto,FieldName,Type,Id,Required)

#define HDU_V2_REPEATED_FIELD_DEF5(FieldName,Type,Id,Required,Default) \
    HDU_V2_FIELD_DEF_REPEATED(RepeatedMode::Auto,FieldName,Type,Id,Required,Default)

#define HDU_V2_REPEATED_FIELD_DEF6(FieldName,Type,Id,Required,Default,Mode)\
    HDU_V2_FIELD_DEF_REPEATED(RepeatedMode:: Mode,FieldName,Type,Id,Required,Default)

#define HDU_V2_VARG_SELECT_REPEATED_FIELD(...) \
    HDU_V2_EXPAND(HDU_V2_GET_ARG7(__VA_ARGS__, \
                                  HDU_V2_REPEATED_FIELD_DEF6, \
                                  HDU_V2_REPEATED_FIELD_DEF5, \
                                  HDU_V2_REPEATED_FIELD_DEF4, \
                                  HDU_V2_REPEATED_FIELD_DEF3  \
                                  ))

#define HDU_V2_REPEATED_FIELD(...) HDU_V2_EXPAND(HDU_V2_VARG_SELECT_REPEATED_FIELD(__VA_ARGS__)(__VA_ARGS__))

#define HDU_V2_REPEATED_BASIC_FIELD(FieldName,Type,Id,Required,Default) \
    HDU_V2_IS_BASIC_TYPE(FieldName,Type,Auto) \
    HDU_V2_REPEATED_FIELD_DEF5(FieldName,Type,Id,Required,Default)

#define HDU_V2_REPEATED_UNIT_FIELD(FieldName,Type,Id,Required) \
    HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    HDU_V2_REPEATED_FIELD_DEF4(FieldName,Type,Id,Required)

//---------------------------------------------------------------

#define HDU_V2_ENUM(EnumName,...) \
enum class EnumName : int {__VA_ARGS__};

#define HDU_V2_TYPE_ENUM(Type) TYPE_ENUM<Type>

#define HDU_V2_TYPE_FIXED_STRING(Length) \
    TYPE_FIXED_STRING<Length>

//---------------------------------------------------------------

#define HDU_V2_UNIT_BEGIN(UnitName) \
    namespace UnitName { \
    constexpr auto base_fields=boost::hana::make_tuple();

#define HDU_V2_UNIT_BODY(UnitName,...) \
    HATN_IGNORE_UNUSED_CONST_VARIABLE_BEGIN \
    HATN_IGNORE_UNUSED_VARIABLE_BEGIN \
    using namespace HATN_DATAUNIT_NAMESPACE; \
    using namespace HATN_DATAUNIT_NAMESPACE::types; \
    using namespace HATN_DATAUNIT_META_NAMESPACE; \
    struct conf{constexpr static const char* name=#UnitName;};\
    struct c{};\
    HATN_COUNTER_MAKE(c);\
    template <int N> struct field{};\
    template <int N> struct field_id_{};\
    template <> struct field_id_<0>{using hana_tag=field_id_tag;}; \
    __VA_ARGS__ \
    namespace {HATN_MAYBE_CONSTEXPR auto field_defs=concat_fields(base_fields,make_fields_tuple<field,HATN_COUNTER_GET(c)>());}\
    static_assert(decltype(check_ids_unique(field_defs))::value,"Field IDs must be unique");\
    static_assert(decltype(check_names_unique(field_defs))::value,"Field names must be unique");\
    namespace {HATN_MAYBE_CONSTEXPR auto unit_c=unit<conf>::type_c(field_defs);}\
    using unit_base_t=decltype(unit_c)::type;\
    using type=unit_t<unit_base_t>;\
    using managed=managed_unit<type>;\
    /* types below are explicitly derived instead of just "using" in order to decrease object code size */ \
    struct field_id_s_t : public field_id_<HATN_COUNTER_GET(c)>{};\
    constexpr field_id_s_t fields{};\
    struct traits : public unit_traits<type,managed,field_id_s_t>{};\
    using shared_traits=traits; \
    struct TYPE : public subunit<traits>{}; \
    }\
    HATN_IGNORE_UNUSED_CONST_VARIABLE_END \
    HATN_IGNORE_UNUSED_VARIABLE_END

#define HDU_V2_UNIT(UnitName,...) \
    HDU_V2_UNIT_BEGIN(UnitName) \
    HDU_V2_UNIT_BODY(UnitName,__VA_ARGS__)

#define HDU_V2_BASE(UnitName) UnitName::field_defs

#define HDU_V2_UNIT_WITH_BEGIN(UnitName, Base) \
    namespace UnitName { \
    constexpr auto base_fields=HATN_DATAUNIT_META_NAMESPACE::concat_fields Base;

#define HDU_V2_UNIT_WITH(UnitName, Base, ...) \
    HDU_V2_UNIT_WITH_BEGIN(UnitName, Base) \
    HDU_V2_UNIT_BODY(UnitName,__VA_ARGS__)

#define HDU_V2_UNIT_EMPTY(UnitName) \
    HATN_IGNORE_UNUSED_CONST_VARIABLE_BEGIN \
    HATN_IGNORE_UNUSED_VARIABLE_BEGIN \
    namespace UnitName { \
    using namespace hatn::dataunit; \
    using namespace hatn::dataunit::types; \
    using namespace hatn::dataunit::meta; \
    struct conf{constexpr static const char* name=#UnitName;};\
    constexpr auto field_defs=boost::hana::make_tuple();\
    using type=EmptyUnit<conf>;\
    using managed=EmptyManagedUnit<conf>;\
    struct fields_t{};\
    constexpr fields_t fields{};\
    struct traits : public unit_traits<type,managed,fields_t>{};\
    using shared_traits=traits; \
    struct TYPE : public subunit<traits>{}; \
    }\
    HATN_IGNORE_UNUSED_CONST_VARIABLE_END \
    HATN_IGNORE_UNUSED_VARIABLE_END

//---------------------------------------------------------------

#define HDU_V2_INSTANTIATE1(UnitName) \
    template class HATN_DATAUNIT_META_NAMESPACE::unit_t<UnitName::unit_base_t>;\
    template class HATN_DATAUNIT_NAMESPACE::ManagedUnit<UnitName::type>;\
    template class HATN_COMMON_NAMESPACE::WithStaticAllocator<UnitName::managed>;

#ifndef __MINGW32__

    #define HDU_V2_EXPORT(UnitName,Export) \
        template class Export HATN_DATAUNIT_META_NAMESPACE::unit_t<UnitName::unit_base_t>;\
        template class Export HATN_DATAUNIT_NAMESPACE::ManagedUnit<UnitName::type>;\
        template class Export HATN_COMMON_NAMESPACE::WithStaticAllocator<UnitName::managed>;

#else

    #define HDU_V2_EXPORT(UnitName,Export) HDU_V2_INSTANTIATE1(UnitName)

#endif

#define HDU_V2_GET_ARG3(arg1, arg2, arg3, ...) arg3

#define HDU_V2_VARG_SELECT_INST(...) \
    HDU_V2_EXPAND(HDU_V2_GET_ARG3(__VA_ARGS__, \
                              HDU_V2_EXPORT, \
                              HDU_V2_INSTANTIATE1\
                              ))

#define HDU_V2_INSTANTIATE(...) HDU_V2_EXPAND(HDU_V2_VARG_SELECT_INST(__VA_ARGS__)(__VA_ARGS__))

#define HDU_V2_INSTANTIATE_EMPTY(UnitName) \
    template class HATN_DATAUNIT_NAMESPACE::EmptyUnit<UnitName::conf>;

#ifdef _MSC_VER

    #define HDU_V2_EXPORT_EMPTY(UnitName,Export) \
        template class Export HATN_DATAUNIT_NAMESPACE::EmptyUnit<UnitName::conf>;

#else

    #define HDU_V2_EXPORT_EMPTY(UnitName,Export) HDU_V2_INSTANTIATE_EMPTY(UnitName)

#endif

#endif // HATNDATAUNITMACROS_H
