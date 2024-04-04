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
static_assert(decltype(meta::is_unit_type<Type>())::value || decltype(meta::is_basic_type<Type>())::value,"Invalid field type for "#FieldName);

#define HDU_V2_CHECK_ID(FieldName,Id) \
    static_assert(std::is_integral<decltype(Id)>::value && Id>0,"ID must be positive integer for "#FieldName);

//---------------------------------------------------------------

#define HDU_V2_DEFAULT_PREPARE(FieldName,Type,Default) \
    struct default_##FieldName\
    {\
            constexpr static auto value=Default;\
            using type=decltype(value);\
    };

//---------------------------------------------------------------

#define HDU_V2_FIELD_DEF(FieldName,Type,Id,Default,required,repeated_traits) \
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
                       hana::int_<HATN_COUNTER_GET(c)>,\
                       Type,\
                       default_type<Type,default_##FieldName>,\
                       required,\
                       decltype(repeated_traits)\
                       >;\
        using type=typename traits::type;\
        using shared_type=typename traits::shared_type;\
        HDU_V2_FIELD_NAME_STR(FieldName)\
        constexpr static auto id=hana::int_c<Id>;\
    };\
    constexpr typename field<HATN_COUNTER_GET(c)>::traits FieldName{};\
    constexpr typename field<HATN_COUNTER_GET(c)>::traits inst_##FieldName{};\
    template <> struct field_id<HATN_COUNTER_GET(c)+1> : public field_id<HATN_COUNTER_GET(c)>\
    {\
        constexpr static const auto& FieldName=inst_##FieldName;\
    };\
    HATN_COUNTER_INC(c);

//---------------------------------------------------------------

#define HDU_V2_EXPAND(x) x
#define HDU_V2_GET_ARG6(arg1, arg2, arg3, arg4, arg5, arg6, ...) arg6
#define HDU_V2_GET_ARG8(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, ...) arg8

//---------------------------------------------------------------

#define HDU_V2_FIELD_DEF_OPTIONAL(FieldName,Type,Id,Repeated) \
    HDU_V2_IS_FIELD_TYPE(FieldName,Type) \
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Auto,hana::bool_<false>,Repeated)

#define HDU_V2_OPTIONAL_FIELD(FieldName,Type,Id) HDU_V2_FIELD_DEF_OPTIONAL(FieldName,Type,Id,Auto)

#define HDU_V2_FIELD_DEF_REQUIRED(FieldName,Type,Id,Required,Repeated) \
    HDU_V2_IS_FIELD_TYPE(FieldName,Type) \
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Auto,hana::bool_<Required>,Repeated)

#define HDU_V2_REQUIRED_FIELD(FieldName,Type,Id) HDU_V2_FIELD_DEF_REQUIRED(FieldName,Type,Id,true,Auto)

#define HDU_V2_FIELD_DEF_DEFAULT(FieldName,Type,Id,Required,Default,Repeated) \
    HDU_V2_IS_BASIC_TYPE(FieldName,Type,Repeated) \
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Default,hana::bool_<Required>,Repeated)

#define HDU_V2_DEFAULT_FIELD(FieldName,Type,Id,Default) HDU_V2_FIELD_DEF_DEFAULT(FieldName,Type,Id,false,Default,Auto)

#define HDU_V2_VARG_SELECT_FIELD(...) \
    HDU_V2_EXPAND(HDU_V2_GET_ARG6(__VA_ARGS__, \
                                    HDU_V2_FIELD_DEF_DEFAULT, \
                                    HDU_V2_FIELD_DEF_REQUIRED, \
                                    HDU_V2_FIELD_DEF_OPTIONAL \
                                  ))

#define HDU_V2_FIELD(...) HDU_V2_EXPAND(HDU_V2_VARG_SELECT_FIELD(__VA_ARGS__)(__VA_ARGS__,Auto))

//---------------------------------------------------------------

#define HDU_V2_FIELD_DEF_REPEATED(Mode,ContentType,FieldName,...) \
    constexpr repeated_config<Mode,ContentType> cfg_##FieldName{};\
    HDU_V2_EXPAND(HDU_V2_VARG_SELECT_FIELD(FieldName,__VA_ARGS__)(FieldName,__VA_ARGS__,cfg_##FieldName))

#define HDU_V2_REPEATED_FIELD_DEF3(FieldName,Type,Id) \
    HDU_V2_FIELD_DEF_REPEATED(RepeatedMode::Auto,RepeatedContentType::Auto,FieldName,Type,Id)

#define HDU_V2_REPEATED_FIELD_DEF4(FieldName,Type,Id,Required) \
    HDU_V2_FIELD_DEF_REPEATED(RepeatedMode::Auto,RepeatedContentType::Auto,FieldName,Type,Id,Required)

#define HDU_V2_REPEATED_FIELD_DEF5(FieldName,Type,Id,Required,Default) \
    HDU_V2_FIELD_DEF_REPEATED(RepeatedMode::Auto,RepeatedContentType::Auto,FieldName,Type,Id,Required,Default)

#define HDU_V2_REPEATED_FIELD_DEF6(FieldName,Type,Id,Required,Default,Mode)\
    HDU_V2_FIELD_DEF_REPEATED(RepeatedMode:: Mode,RepeatedContentType::Auto,FieldName,Type,Id,Required,Default)

#define HDU_V2_REPEATED_FIELD_DEF7(FieldName,Type,Id,Required,Default,Mode,ContentType)\
    HDU_V2_FIELD_DEF_REPEATED(RepeatedMode:: Mode,RepeatedContentType:: ContentType,FieldName,Type,Id,Required,Default)

#define HDU_V2_VARG_SELECT_REPEATED_FIELD(...) \
    HDU_V2_EXPAND(HDU_V2_GET_ARG8(__VA_ARGS__, \
                                  HDU_V2_REPEATED_FIELD_DEF7, \
                                  HDU_V2_REPEATED_FIELD_DEF6, \
                                  HDU_V2_REPEATED_FIELD_DEF5, \
                                  HDU_V2_REPEATED_FIELD_DEF4, \
                                  HDU_V2_REPEATED_FIELD_DEF3  \
                                  ))

#define HDU_V2_REPEATED_FIELD(...) HDU_V2_EXPAND(HDU_V2_VARG_SELECT_REPEATED_FIELD(__VA_ARGS__)(__VA_ARGS__))


#define HDU_V2_REPEATED_FIELD_NORMAL(FieldName,Type,Id,Required,Default) \
    HDU_V2_IS_BASIC_TYPE(FieldName,Type,Auto) \
    HDU_V2_REPEATED_FIELD_DEF5(FieldName,Type,Id,Required,Default)

#define HDU_V2_REPEATED_UNIT_FIELD(FieldName,Type,Id,Required) \
    HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    HDU_V2_REPEATED_FIELD_DEF4(FieldName,Type,Id,Required)

#define HDU_V2_REPEATED_EXTERNAL_UNIT_FIELD(FieldName,Type,Id,Required) \
    HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    HDU_V2_REPEATED_FIELD_DEF7(FieldName,Type,Id,Required,Auto,Auto,External)

#define HDU_V2_REPEATED_EMBEDDED_UNIT_FIELD(FieldName,Type,Id,Required) \
    HDU_V2_IS_UNIT_TYPE(FieldName,Type) \
    HDU_V2_REPEATED_FIELD_DEF7(FieldName,Type,Id,Required,Auto,Auto,Embedded)


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
    namespace {auto field_defs=boost::hana::concat(base_fields,make_fields_tuple<field,HATN_COUNTER_GET(c)>());}\
    static_assert(decltype(check_ids_unique(field_defs))::value,"Field IDs must be unique");\
    static_assert(decltype(check_names_unique(field_defs))::value,"Field names must be unique");\
    namespace {auto unit_c=unit<conf>::type_c(field_defs);}\
    namespace {auto shared_unit_c=unit<conf>::shared_type_c(field_defs);}\
    using fields_t=field_id<HATN_COUNTER_GET(c)>;\
    using unit_base_t=decltype(unit_c)::type;\
    using unit_shared_base_t=decltype(shared_unit_c)::type;\
    using type=unit_t<unit_base_t>;\
    using shared_type=unit_t<unit_shared_base_t,hana::true_>;\
    using managed=managed_unit<type>;\
    using shared_managed=shared_managed_unit<shared_type>;\
    /* types below are explicitly derived instead of just "using" in order to decrease object code size */ \
    struct fields : public fields_t{};\
    struct traits : public unit_traits<type,managed,fields>{};\
    struct shared_traits : public unit_traits<shared_type,shared_managed,fields>{};\
    struct TYPE : public subunit<traits,shared_traits>{}; \
    }\
    HATN_IGNORE_UNUSED_CONST_VARIABLE_END

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
    namespace UnitName { \
    using namespace hatn::dataunit; \
    using namespace hatn::dataunit::types; \
    using namespace hatn::dataunit::meta; \
    struct conf{constexpr static const char* name=#UnitName;};\
    constexpr auto field_defs=boost::hana::make_tuple();\
    using type=EmptyUnit<conf>;\
    using shared_type=type;\
    using managed=EmptyManagedUnit<conf>;\
    using shared_managed=managed;\
    struct fields{};\
    struct traits : public unit_traits<type,managed,fields>{};\
    struct shared_traits : public unit_traits<shared_type,shared_managed,fields>{};\
    struct TYPE : public subunit<traits,shared_traits>{}; \
    }

//---------------------------------------------------------------

#define HDU_V2_INSTANTIATE1(UnitName) \
    template class HATN_DATAUNIT_META_NAMESPACE::unit_t<UnitName::unit_base_t>;\
    template class HATN_DATAUNIT_META_NAMESPACE::unit_t<UnitName::unit_shared_base_t,boost::hana::true_>;\
    template class HATN_DATAUNIT_META_NAMESPACE::managed_unit<UnitName::type>;\
    template class HATN_DATAUNIT_META_NAMESPACE::shared_managed_unit<UnitName::shared_type>;\
    template class HATN_COMMON_NAMESPACE::WithStaticAllocator<UnitName::managed>; \
    template class HATN_COMMON_NAMESPACE::WithStaticAllocator<UnitName::shared_managed>;

#ifdef _MSC_VER

    #define HDU_V2_EXPORT(UnitName,Export) \
        template class Export HATN_DATAUNIT_META_NAMESPACE::unit_t<UnitName::unit_base_t>;\
        template class Export HATN_DATAUNIT_META_NAMESPACE::unit_t<UnitName::unit_shared_base_t,boost::hana::true_>;\
        template class Export HATN_DATAUNIT_META_NAMESPACE::managed_unit<UnitName::type>;\
        template class Export HATN_DATAUNIT_META_NAMESPACE::shared_managed_unit<UnitName::shared_type>;\
        template class Export HATN_COMMON_NAMESPACE::WithStaticAllocator<UnitName::managed>; \
        template class Export HATN_COMMON_NAMESPACE::WithStaticAllocator<UnitName::shared_managed>;

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

#endif // HATNDATAUNITMACROS_H
