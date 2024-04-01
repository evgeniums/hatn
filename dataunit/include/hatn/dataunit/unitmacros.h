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
    HDU_V2_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,false)

#define HDU_V2_REQUIRED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
    HDU_V2_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,true)

#define HDU_V2_DEFAULT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Default) \
    HDU_V2_DEFAULT_TRAITS(FieldName,Type,Default)\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,default_##FieldName,hana::false_,hana::false_)

#define HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,RepeatedConfig,Required,Default) \
    HDU_V2_DEFAULT_TRAITS(FieldName,Type,Default)\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,default_##FieldName,Required,RepeatedConfig)

#define HDU_V2_REPEATED_FIELD_NORMAL_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required,Default) \
    using cfg_##FieldName=repeated_config<RepeatedMode::Normal>;\
    HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,cfg_##FieldName,hana::bool_<Required>,Default)

#define HDU_V2_REPEATED_FIELD_PBPACKED_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required,Default) \
    using cfg_##FieldName=repeated_config<RepeatedMode::ProtobufPacked>;\
    HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,cfg_##FieldName,hana::bool_<Required>,Default)

#define HDU_V2_REPEATED_FIELD_PBORDINARY_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required,Default) \
    using cfg_##FieldName=repeated_config<RepeatedMode::ProtobufOrdinary>;\
    HDU_V2_REPEATED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,cfg_##FieldName,hana::bool_<Required>,Default)

#define HDU_V2_UNIT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    HDU_V2_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required)

#define HDU_V2_REPEATED_UNIT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    using cfg_##FieldName=repeated_config<RepeatedMode::Normal,RepeatedContentType::AutoDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_REPEATED_EXTERNAL_UNIT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
using cfg_##FieldName=repeated_config<RepeatedMode::Normal,RepeatedContentType::ExternalDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_REPEATED_EMBEDDED_UNIT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
using cfg_##FieldName=repeated_config<RepeatedMode::Normal,RepeatedContentType::EmbeddedDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_REPEATED_UNIT_FIELD_PBORDINARY_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
using cfg_##FieldName=repeated_config<RepeatedMode::ProtobufOrdinary,RepeatedContentType::AutoDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_REPEATED_EXTERNAL_UNIT_FIELD_PBORDINARY_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    using cfg_##FieldName=repeated_config<RepeatedMode::ProtobufOrdinary,RepeatedContentType::ExternalDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_REPEATED_EMBEDDED_UNIT_FIELD_PBORDINARY_WITH_DESCRIPTION(FieldName,Type,Id,Description,Required) \
    using cfg_##FieldName=repeated_config<RepeatedMode::ProtobufOrdinary,RepeatedContentType::EmbeddedDataunit>;\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::bool_<Required>,cfg_##FieldName)

#define HDU_V2_ENUM(EnumName,...) \
enum class EnumName : int {__VA_ARGS__};

#define HDU_V2_TYPE_ENUM(Type) TYPE_ENUM<Type>

#define HDU_V2_TYPE_FIXED_STRING(Length) \
    TYPE_FIXED_STRING<Length>

#define HDU_V2_DATAUNIT(UnitName,...) \
    namespace UnitName { \
        using namespace hatn::dataunit; \
        using namespace hatn::dataunit::types; \
        using namespace hatn::dataunit::meta; \
        struct conf{constexpr static const char* name=#UnitName;};\
        struct c{};\
        HATN_COUNTER_MAKE(c);\
        template <int N> struct field{};\
        template <int N> struct field_id{};\
        template <> struct field_id<0>{using hana_tag=field_id_tag;}; \
        __VA_ARGS__ \
        auto field_defs=make_fields_tuple<field,HATN_COUNTER_GET(c)>();\
        auto unit_c=unit<conf>::type_c(field_defs);\
        auto shared_unit_c=unit<conf>::shared_type_c(field_defs);\
        using fields=field_id<HATN_COUNTER_GET(c)>;\
        constexpr fields fields_instance{};\
        using type=unit_t<decltype(unit_c)::type>;\
        using shared_type=unit_t<decltype(shared_unit_c)::type>;\
        using managed=managed_unit<type>;\
        using shared_managed=shared_managed_unit<shared_type>;\
        struct traits\
        {\
                constexpr static const auto& fields=fields_instance;\
                using type=UnitName::type;\
                using managed=UnitName::managed;\
        };\
        struct shared_traits\
        {\
            constexpr static const auto& fields=fields_instance;\
            using type=UnitName::shared_type;\
            using managed=UnitName::managed;\
        };\
        struct TYPE : public TYPE_DATAUNIT\
        {\
            using type=traits::type;\
            using shared_type=HATN_COMMON_NAMESPACE::SharedPtr<shared_traits::managed>;\
            using base_shared_type=shared_traits::type;\
            using Hatn=std::true_type;\
            constexpr static const bool isSizeIterateNeeded=true;\
            template <typename ...Args>\
            static shared_type createManagedObject(AllocatorFactory* factory, Unit* unitBase)\
            {\
                if (factory==nullptr) factory=unitBase->factory();\
                auto m=factory->createObject<typename shared_traits::managed>(factory);\
                return m;\
            }\
        };\
        /*using TYPE=subunit;*/\
        /*using traits=unit_traits<type,managed,fields>;*/\
        /*using shared_traits=unit_traits<shared_type,shared_managed,fields>;*/\
        /*using TYPE=subunit<traits,shared_traits>;*/\
}

#endif // HATNDATAUNITMACROS_H
