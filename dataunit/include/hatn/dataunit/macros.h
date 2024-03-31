/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/macros.h
  *
  *      Definitions of macros used to declare data units
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITMACROS_H
#define HATNDATAUNITMACROS_H

#include <hatn/dataunit/unittraits.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

#define _DSC_DU_EXPAND(x) x
#define _HDU_FIELD_DEF_GET_ARG5(arg1, arg2, arg3, arg4, arg5, ...) arg5
#define _HDU_FIELD_DEF_GET_ARG6(arg1, arg2, arg3, arg4, arg5, arg6, ...) arg6

#define _HDU_DATAUNIT_PREPARE_CHECKS() \
    HATN_IGNORE_UNUSED_VARIABLE_BEGIN \
    HATN_IGNORE_UNUSED_CONST_VARIABLE_BEGIN \
    template <typename, typename = void> struct CheckUnitType{ constexpr static bool ok(){ return false;} }; \
    template <typename Type> struct CheckUnitType<Type,std::enable_if_t<Type::isUnitType::value>>{constexpr static bool ok(){ return true;}}; \
    template <typename, typename = void> struct CheckBasicType{ constexpr static bool ok(){ return false;} }; \
    template <typename Type> struct CheckBasicType<Type,std::enable_if_t<Type::BasicType::value>>{constexpr static bool ok(){ return true;}}; \

#define _HDU_DATAUNIT_DECLARE_TRAITS() \
        constexpr const fields_impl fields_instance{}; \
        struct shared_traits \
        { \
            constexpr static const auto& fields=fields_instance; \
            using type=shared_fields_type; \
            using managed=shared_fields_managed_type;  \
        }; \
        struct traits \
        { \
            constexpr static const auto& fields=fields_instance; \
            using type=type_impl; \
            using managed=managed_impl; \
        }; \
        struct TYPE : public ::hatn::dataunit::types::TYPE_DATAUNIT \
        { \
            using type=typename traits::type; \
            using shared_type=::hatn::common::SharedPtr<typename shared_traits::managed>; \
            using base_shared_type=typename shared_traits::type; \
            using Hatn=std::true_type; \
            constexpr static const bool isSizeIterateNeeded=true; \
            template <typename ...Args> static shared_type createManagedObject(::hatn::dataunit::AllocatorFactory* factory, ::hatn::dataunit::Unit* unitBase) \
            { \
                if (factory==nullptr) factory=unitBase->factory(); \
                auto m=factory->createObject<typename shared_traits::managed>(factory); \
                return m; \
            } \
        };

#define _HDU_DATAUNIT_END() \
        _HDU_DATAUNIT_DECLARE_TRAITS() \
        template <int N> struct extension{}; \
        HATN_IGNORE_UNUSED_CONST_VARIABLE_END \
        HATN_IGNORE_UNUSED_VARIABLE_END

#define _HDU_DATAUNIT_PREPARE(UnitName) \
    template <int N> struct field{}; \
    struct Conf {constexpr static const char* name=#UnitName;}; \
    template <> struct field<HATN_GET_COUNT(Fields)>{using type=Conf;using shared=Conf;constexpr static const int index=HATN_GET_COUNT(Fields);}; \
    template <int N> struct _FieldTypes {}; \
    template <> struct _FieldTypes<0> {}; \
    HATN_INC_COUNT(Fields); \
    template <typename ...Types> struct VariadicTypedef \
    {}; \
    \
    template <template <int> class SingleType, int N, template <typename ...> class T, typename ...Types> \
        struct Concat \
    { \
        using type=typename Concat<SingleType,N-1,T,VariadicTypedef<typename SingleType<N>::type,Types...>>::type; \
    }; \
    \
    template <template <int> class SingleType, int N, template <typename ...> class T, typename ...Types> \
        struct Concat<SingleType,N,T,VariadicTypedef<Types...>> \
    { \
        using type=typename Concat<SingleType,N,T,Types...>::type; \
    }; \
    \
    template <template <int> class SingleType, template <typename ...> class T, typename ...Types> \
        struct Concat<SingleType,0,T,Types...> \
    { \
        using type=T<typename SingleType<0>::type,Types...>; \
    }; \
    \
    template <template <int> class SingleType, template <typename ...> class T, typename ...Types> \
        struct Concat<SingleType,0,T,VariadicTypedef<Types...>> \
    { \
        using type=typename Concat<SingleType,0,T,Types...>::type; \
    }; \
    template <template <int> class SingleType, int N, template <typename ...> class T, typename ...Types> \
        struct ConcatShared \
    { \
        using type=typename ConcatShared<SingleType,N-1,T,VariadicTypedef<typename SingleType<N>::shared,Types...>>::type; \
    }; \
    \
    template <template <int> class SingleType, int N, template <typename ...> class T, typename ...Types> \
        struct ConcatShared<SingleType,N,T,VariadicTypedef<Types...>> \
    { \
        using type=typename ConcatShared<SingleType,N,T,Types...>::type; \
    }; \
    \
    template <template <int> class SingleType, template <typename ...> class T, typename ...Types> \
        struct ConcatShared<SingleType,0,T,Types...> \
    { \
        using type=T<typename SingleType<0>::shared,Types...>; \
    }; \
    \
    template <template <int> class SingleType, template <typename ...> class T, typename ...Types> \
        struct ConcatShared<SingleType,0,T,VariadicTypedef<Types...>> \
    { \
        using type=typename ConcatShared<SingleType,0,T,Types...>::type; \
    }; \

#define _HDU_DATAUNIT_CREATE_TYPE_IMPL_(type,base) \
    class HDU_DATAUNIT_EXPORT type : public base \
    { \
        public: \
            virtual const Field* fieldById(int id) const override; \
            virtual Field* fieldById(int id) override; \
            virtual bool iterateFields(const Unit::FieldVisitor& visitor) override; \
            virtual bool iterateFieldsConst(const Unit::FieldVisitorConst& visitor) const override; \
            virtual size_t fieldCount() const noexcept override; \
            virtual const char* name() const noexcept override; \
            type(AllocatorFactory* factory=AllocatorFactory::getDefault()); \
            inline const type& value() const noexcept \
            { \
                return *this; \
            } \
            inline type* mutableValue() noexcept \
            { \
                return this; \
            } \
            inline type* castToUnit(::hatn::dataunit::Unit* unit) const noexcept \
            { \
                return ::hatn::common::dynamicCastWithSample(unit,this); \
            } \
            virtual int serialize(WireData& wired,bool topLevel=true) const override; \
            /*virtual int serialize(WireBufSolid& wired,bool topLevel=true) const override;*/ \
            /*virtual int serialize(WireBufSolidShared& wired,bool topLevel=true) const override;*/ \
            /*virtual int serialize(WireBufChained& wired,bool topLevel=true) const override;*/ \
            virtual bool parse(WireData& wired,bool topLevel=true) override; \
            virtual bool parse(WireBufSolid& wired,bool topLevel=true) override; \
            /*virtual bool parse(WireBufSolidShared& wired,bool topLevel=true) override;*/ \
    };

#define _HDU_DATAUNIT_IMPLEMENT_TYPE_IMPL_(type,base) \
        const Field* type::fieldById(int id) const {return this->doFieldById(id);} \
        Field* type::fieldById(int id) {return this->doFieldById(id);} \
        bool type::iterateFields(const Unit::FieldVisitor& visitor) {return this->iterate(visitor);}; \
        bool type::iterateFieldsConst(const Unit::FieldVisitorConst& visitor) const {return this->iterateConst(visitor);} \
        size_t type::fieldCount() const noexcept {return this->doFieldCount();} \
        const char* type::name() const noexcept {return this->unitName();} \
        type::type(::hatn::dataunit::AllocatorFactory* factory):base(factory){} \
        int type::serialize(WireData& wired,bool topLevel) const{return io::serialize(*this,wired,topLevel);} \
        /*int type::serialize(WireBufSolid& wired,bool topLevel) const{return io::serialize(*this,wired,topLevel);}*/ \
        /*int type::serialize(WireBufSolidShared& wired,bool topLevel) const{return io::serialize(*this,wired,topLevel);}*/ \
        /*int type::serialize(WireBufChained& wired,bool topLevel) const{return io::serialize(*this,wired,topLevel);}*/ \
        bool type::parse(WireData& wired,bool topLevel) {return io::deserialize(*this,wired,topLevel);} \
        bool type::parse(WireBufSolid& wired,bool topLevel) {return io::deserialize(*this,wired,topLevel);} \
        // bool type::parse(WireBufSolidShared& wired,bool topLevel) {return io::deserialize(*this,wired,topLevel);}

#define _HDU_DATAUNIT_CREATE_TYPE_IMPL() \
    _HDU_DATAUNIT_CREATE_TYPE_IMPL_(type_impl,_type_impl) \
    _HDU_DATAUNIT_CREATE_TYPE_IMPL_(shared_fields_type,_shared_fields_type)

#define _HDU_DATAUNIT_CREATE_TYPES() \
    using fields_impl=_FieldTypes<HATN_GET_COUNT(Fields)-1>; \
    using _type_impl=Concat<field,HATN_GET_COUNT(Fields)-1,DataUnit>::type; \
    using _shared_fields_type=ConcatShared<field,HATN_GET_COUNT(Fields)-1,DataUnit>::type; \
    _HDU_DATAUNIT_CREATE_TYPE_IMPL() \
    HATN_WITH_STATIC_ALLOCATOR_INLINE \
    HATN_WITH_STATIC_ALLOCATOR_DECLARE(managed_impl,HDU_DATAUNIT_EXPORT) \
    class managed_impl : public ::hatn::dataunit::ManagedUnit<type_impl>, \
                         public WithStaticAllocator<managed_impl> \
    { \
        public: \
            using ::hatn::dataunit::ManagedUnit<type_impl>::ManagedUnit; \
    }; \
    using type=type_impl;  \
    using fields=fields_impl;  \
    using managed=managed_impl;  \
    HATN_WITH_STATIC_ALLOCATOR_DECLARE(shared_fields_managed_type,HDU_DATAUNIT_EXPORT) \
    class shared_fields_managed_type : public ::hatn::dataunit::ManagedUnit<shared_fields_type>, \
                         public WithStaticAllocator<shared_fields_managed_type> \
    { \
        public: \
            using ::hatn::dataunit::ManagedUnit<shared_fields_type>::ManagedUnit; \
    };

//! Use this macro to declare DataUnit UnitName
#define _HDU_DATAUNIT(UnitName,...) \
    namespace UnitName { \
        using namespace hatn::dataunit; \
        using namespace hatn::dataunit::types; \
        HATN_PREPARE_COUNTERS() \
        HATN_MAKE_COUNTER(Fields); \
        HATN_MAKE_COUNTER(Extensions); \
        HATN_MAKE_COUNTER(ExtensionsInst); \
        _HDU_DATAUNIT_PREPARE_CHECKS() \
        _HDU_DATAUNIT_PREPARE(UnitName) \
        __VA_ARGS__ \
        _HDU_DATAUNIT_CREATE_TYPES() \
        _HDU_DATAUNIT_END() \
    }

//! Use this macro to declare UnitName without fields
#define _HDU_DATAUNIT_EMPTY(UnitName) \
    namespace UnitName { \
        using namespace hatn::dataunit; \
        using namespace hatn::dataunit::types; \
        HATN_PREPARE_COUNTERS() \
        HATN_MAKE_COUNTER(Fields); \
        HATN_MAKE_COUNTER(Extensions); \
        HATN_MAKE_COUNTER(ExtensionsInst); \
        _HDU_DATAUNIT_PREPARE_CHECKS() \
        _HDU_DATAUNIT_PREPARE(UnitName) \
        using _type_impl=EmptyUnit<Conf>;  \
        using _shared_fields_type=_type_impl; \
        _HDU_DATAUNIT_CREATE_TYPE_IMPL() \
        using type=type_impl;  \
        using managed_impl=EmptyManagedUnit<Conf>;  \
        using fields_impl=_FieldTypes<0>; \
        using shared_fields_managed_type=managed_impl; \
        _HDU_DATAUNIT_END() \
    }

//! Use this macro to extend DataUnit with new fields
#define _HDU_EXTEND(UnitName,ExtensionName,...) \
    HATN_IGNORE_UNUSED_VARIABLE_BEGIN \
    HATN_IGNORE_UNUSED_CONST_VARIABLE_BEGIN \
    namespace UnitName { \
        using namespace hatn::dataunit; \
        using namespace hatn::dataunit::types; \
        __VA_ARGS__ \
        namespace _Ext##ExtensionName \
        { \
            _HDU_DATAUNIT_CREATE_TYPES() \
            _HDU_DATAUNIT_DECLARE_TRAITS() \
        } \
        template <> struct extension<HATN_GET_COUNT(Extensions)> \
        {\
            using fields_impl=_Ext##ExtensionName::fields_impl; \
            using _type_impl=_Ext##ExtensionName::_type_impl; \
            using _shared_fields_type=_Ext##ExtensionName::_shared_fields_type; \
            using managed_impl=_Ext##ExtensionName::managed_impl; \
            using type=_Ext##ExtensionName::type;  \
            using fields=_Ext##ExtensionName::fields;  \
            using managed=_Ext##ExtensionName::managed;  \
            using shared_fields_managed_type=_Ext##ExtensionName::shared_fields_managed_type; \
            using shared_traits=_Ext##ExtensionName::shared_traits; \
            using traits=_Ext##ExtensionName::traits; \
            using TYPE=_Ext##ExtensionName::TYPE; \
        }; \
        using ExtensionName=extension<HATN_GET_COUNT(Extensions)>;\
        HATN_INC_COUNT(Extensions); \
    } \
    HATN_IGNORE_UNUSED_CONST_VARIABLE_END \
    HATN_IGNORE_UNUSED_VARIABLE_END

//! Use this macro to reserve field ID
#define _HDU_FIELD_RESERVE_ID(Id) \
        static_assert(std::is_integral<decltype(Id)>::value,"ID must be integer"); \
        HATN_MAKE_COUNTER(Id); \
        HATN_INC_COUNT(Id); \
        static_assert(HATN_GET_COUNT(Id)==1,"Duplicate field ID");

//! Use this macro to reserve field name
#define _HDU_FIELD_RESERVE_NAME(FieldName,Description) \
    struct a_##FieldName {constexpr static const char* name=#FieldName;constexpr static const char* description=Description;using reserved=std::true_type;};

#define _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        _HDU_FIELD_RESERVE_ID(Id) \
        _HDU_FIELD_RESERVE_NAME(FieldName,Description)

#define _HDU_FIELD_IS_BASIC_TYPE(FieldName,Type,Id) \
        static_assert(CheckBasicType<Type>::ok(),"Field type is not supported");

#define _HDU_FIELD_IS_CUSTOM_TYPE(Type) \
        static_assert(CheckUnitType<Type>::ok(),"Invalid type of dataunit field");

#define _HDU_FIELD_END(FieldName,FieldType,FieldShared,Type_,Id) \
        template <> struct field<HATN_GET_COUNT(Fields)> { \
            using hana_tag=FieldTag;\
            using type=FieldType;\
            using shared=FieldShared;\
            using Type=Type_;\
            constexpr static const char* name() {return FieldType::fieldName();}\
            constexpr static const char* description() {return FieldType::fieldDescription();}\
            constexpr static int id() noexcept {return Id;};\
            constexpr static const int ID=Id;\
            constexpr static const int index=HATN_GET_COUNT(Fields)-1;\
            template <typename T> bool operator ==(const T& other) const \
            { \
                return std::is_same<Type,typename T::Type>::value && id()==other.id(); \
            } \
            template <typename T> bool operator !=(const field& other) const \
            { \
                return !(*this==other); \
            } \
        }; \
        using _f##FieldName=field<HATN_GET_COUNT(Fields)>; \
        constexpr _f##FieldName FieldName{}; \
        constexpr _f##FieldName instance##FieldName{}; \
        template <> struct _FieldTypes<HATN_GET_COUNT(Fields)> : public _FieldTypes<HATN_GET_COUNT(Fields)-1> \
        { \
            constexpr static const auto& FieldName=instance##FieldName; \
        }; \
        HATN_INC_COUNT(Fields);

//! Use this macro to declare an optional dataunit field
#define _HDU_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        _HDU_FIELD_IS_BASIC_TYPE(FieldName,Type,Id) \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=OptionalField<a_##FieldName,Type,Id>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,Type,Id)

#define _HDU_FIELD(FieldName,Type,Id) \
        _HDU_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_WITH_DESCRIPTION, \
                _HDU_FIELD \
                ))
#define _HDU_FIELD_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare a required dataunit field
#define _HDU_FIELD_REQUIRED_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        _HDU_FIELD_IS_BASIC_TYPE(FieldName,Type,Id) \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=RequiredField<a_##FieldName,Type,Id>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,Type,Id)

#define _HDU_FIELD_REQUIRED(FieldName,Type,Id) \
        _HDU_FIELD_REQUIRED_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_REQUIRED_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_REQUIRED_WITH_DESCRIPTION, \
                _HDU_FIELD_REQUIRED \
                ))
#define _HDU_FIELD_REQUIRED_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_REQUIRED_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare an optional dataunit field with default value
#define _HDU_FIELD_DEFAULT_WITH_DESCRIPTION(FieldName,Type,Id,Default,Description) \
        _HDU_FIELD_IS_BASIC_TYPE(FieldName,Type,Id) \
        static_assert(::hatn::common::containsType<Type,TYPE_STRING,TYPE_BYTES>::type::value==std::false_type::value,"Default values can not be set to strings and bytes"); \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        struct CustomDefaultValue##FieldName \
        { \
            static typename Type::type value(){return static_cast<typename Type::type>(Default);} \
            using HasDefV=std::true_type; \
        }; \
        using a__##FieldName=DefaultField<a_##FieldName,Type,Id,CustomDefaultValue##FieldName>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,Type,Id)

#define _HDU_FIELD_DEFAULT(FieldName,Type,Id,Default) \
        _HDU_FIELD_DEFAULT_WITH_DESCRIPTION(FieldName,Type,Id,Default,"")

#define _HDU_FIELD_DEFAULT_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG6(__VA_ARGS__, \
                _HDU_FIELD_DEFAULT_WITH_DESCRIPTION, \
                _HDU_FIELD_DEFAULT \
                ))
#define _HDU_FIELD_DEFAULT_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_DEFAULT_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare a dataunit field
#define _HDU_FIELD_DATAUNIT_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        _HDU_FIELD_IS_CUSTOM_TYPE(Type) \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=EmbeddedUnitField<a_##FieldName,Type,Id>; \
        using a___##FieldName=SharedUnitField<a_##FieldName,Type,Id>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a___##FieldName,Type,Id)

#define _HDU_FIELD_DATAUNIT(FieldName,Type,Id) \
        _HDU_FIELD_DATAUNIT_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_DATAUNIT_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_DATAUNIT_WITH_DESCRIPTION, \
                _HDU_FIELD_DATAUNIT \
                ))
#define _HDU_FIELD_DATAUNIT_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_DATAUNIT_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare an external linked dataunit field
#define _HDU_FIELD_EXTERNAL_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        _HDU_FIELD_IS_CUSTOM_TYPE(Type) \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=SharedUnitField<a_##FieldName,Type,Id>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,Type,Id)

#define _HDU_FIELD_EXTERNAL(FieldName,Type,Id) \
        _HDU_FIELD_EXTERNAL_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_EXTERNAL_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_EXTERNAL_WITH_DESCRIPTION, \
                _HDU_FIELD_EXTERNAL \
                ))
#define _HDU_FIELD_EXTERNAL_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_EXTERNAL_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare an embedded dataunit field with shared embedded comand
#define _HDU_FIELD_EMBEDDED_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        _HDU_FIELD_IS_CUSTOM_TYPE(Type) \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=EmbeddedUnitField<a_##FieldName,Type,Id>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,Type,Id)

#define _HDU_FIELD_EMBEDDED(FieldName,Type,Id) \
        _HDU_FIELD_EMBEDDED_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_EMBEDDED_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_EMBEDDED_WITH_DESCRIPTION, \
                _HDU_FIELD_EMBEDDED \
                ))
#define _HDU_FIELD_EMBEDDED_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_EMBEDDED_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare an enum
#define _HDU_ENUM(EnumName,...) \
        enum class EnumName : int {__VA_ARGS__};

//! Use this macro for enum type
#define _HDU_TYPE_ENUM(Type) TYPE_ENUM<Type>

//! Use this macro for fixed string type
#define _HDU_TYPE_FIXED_STRING(Length) \
    TYPE_FIXED_STRING<Length>

//! Use this macro to declare a repeated dataunit field with strong type check
#define _HDU_FIELD_REPEATED_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        static_assert(CheckBasicType<Type>::ok() || CheckUnitType<Type>::ok(),"Invalid type of repeated field"); \
        static_assert(!std::is_same<Type,TYPE_BOOL>::value,"Repeated field can not be of TYPE_BOOL, use TYPE_UINT8 instead"); \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=RepeatedField<a_##FieldName,Type,Id,RepeatedTraits<Type>>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,RepeatedTraits<Type>,Id)

#define _HDU_FIELD_REPEATED(FieldName,Type,Id) \
        _HDU_FIELD_REPEATED_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_REPEATED_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_REPEATED_WITH_DESCRIPTION, \
                _HDU_FIELD_REPEATED \
                ))
#define _HDU_FIELD_REPEATED_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_REPEATED_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare a repeated dataunit field with default elements
#define _HDU_FIELD_REPEATED_DEFAULT_WITH_DESCRIPTION(FieldName,Type,Id,Default,Description) \
        static_assert(CheckBasicType<Type>::ok() || CheckUnitType<Type>::ok(),"Invalid type of repeated field"); \
        static_assert(!std::is_same<Type,TYPE_BOOL>::value,"Repeated field can not be of TYPE_BOOL, use TYPE_UINT8 instead"); \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        struct CustomDefaultValue##FieldName \
        { \
            static typename Type::type value(){return static_cast<typename Type::type>(Default);} \
            using HasDefV=std::true_type; \
        }; \
        using a__##FieldName=RepeatedField<a_##FieldName,Type,Id,RepeatedTraits<Type>,CustomDefaultValue##FieldName>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,RepeatedTraits<Type>,Id)

#define _HDU_FIELD_REPEATED_DEFAULT(FieldName,Type,Id,Default) \
        _HDU_FIELD_REPEATED_DEFAULT_WITH_DESCRIPTION(FieldName,Type,Id,Default,"")

#define _HDU_FIELD_REPEATED_DEFAULT_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG6(__VA_ARGS__, \
                _HDU_FIELD_REPEATED_DEFAULT_WITH_DESCRIPTION, \
                _HDU_FIELD_REPEATED_DEFAULT \
                ))
#define _HDU_FIELD_REPEATED_DEFAULT_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_REPEATED_DEFAULT_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare a repeated dataunit field
#define _HDU_FIELD_REPEATED_DATAUNIT_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        _HDU_FIELD_IS_CUSTOM_TYPE(Type) \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=RepeatedField<a_##FieldName,EmbeddedUnitFieldTmpl<Type>,Id,RepeatedTraits<Type>>; \
        using ___##FieldName=RepeatedField<a_##FieldName,SharedUnitFieldTmpl<Type>,Id,RepeatedTraits<Type>>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,___##FieldName,RepeatedTraits<Type>,Id)

#define _HDU_FIELD_REPEATED_DATAUNIT(FieldName,Type,Id) \
        _HDU_FIELD_REPEATED_DATAUNIT_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_REPEATED_DATAUNIT_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_REPEATED_DATAUNIT_WITH_DESCRIPTION, \
                _HDU_FIELD_REPEATED_DATAUNIT \
                ))
#define _HDU_FIELD_REPEATED_DATAUNIT_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_REPEATED_DATAUNIT_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare a repeated dataunit field on shared data units
#define _HDU_FIELD_REPEATED_EXTERNAL_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        _HDU_FIELD_IS_CUSTOM_TYPE(Type) \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=RepeatedField<a_##FieldName,SharedUnitFieldTmpl<Type>,Id,RepeatedTraits<Type>>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,RepeatedTraits<Type>,Id)

#define _HDU_FIELD_REPEATED_EXTERNAL(FieldName,Type,Id) \
        _HDU_FIELD_REPEATED_EXTERNAL_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_REPEATED_EXTERNAL_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_REPEATED_EXTERNAL_WITH_DESCRIPTION, \
                _HDU_FIELD_REPEATED_EXTERNAL \
                ))
#define _HDU_FIELD_REPEATED_EXTERNAL_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_REPEATED_EXTERNAL_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare a repeated dataunit field on embedded data units
#define _HDU_FIELD_REPEATED_EMBEDDED_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        _HDU_FIELD_IS_CUSTOM_TYPE(Type) \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=RepeatedField<a_##FieldName,EmbeddedUnitFieldTmpl<Type>,Id,RepeatedTraits<Type>>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,RepeatedTraits<Type>,Id)

#define _HDU_FIELD_REPEATED_EMBEDDED(FieldName,Type,Id) \
        _HDU_FIELD_REPEATED_EMBEDDED_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_REPEATED_EMBEDDED_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_REPEATED_EMBEDDED_WITH_DESCRIPTION, \
                _HDU_FIELD_REPEATED_EMBEDDED \
                ))
#define _HDU_FIELD_REPEATED_EMBEDDED_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_REPEATED_EMBEDDED_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))

//! Use this macro to declare a repeated dataunit field compatible with repeated unpacked type of Google Protocol Buffers for ordinary types
#define _HDU_FIELD_REPEATED_PROTOBUF_ORDINARY_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        static_assert(CheckBasicType<Type>::ok(),"Invalid type of repeated field"); \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=RepeatedFieldProtoBufOrdinary<a_##FieldName,Type,Id,RepeatedTraits<Type>>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,RepeatedTraits<Type>,Id)

#define _HDU_FIELD_REPEATED_PROTOBUF_ORDINARY(FieldName,Type,Id) \
        _HDU_FIELD_REPEATED_PROTOBUF_ORDINARY_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_REPEATED_PROTOBUF_ORDINARY_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_REPEATED_PROTOBUF_ORDINARY_WITH_DESCRIPTION, \
                _HDU_FIELD_REPEATED_PROTOBUF_ORDINARY \
                ))
#define _HDU_FIELD_REPEATED_PROTOBUF_ORDINARY_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_REPEATED_PROTOBUF_ORDINARY_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare a repeated dataunit field compatible with repeated unpacked type of Google Protocol Buffers
#define _HDU_FIELD_REPEATED_DATAUNIT_PROTOBUF_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        _HDU_FIELD_IS_CUSTOM_TYPE(Type) \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=RepeatedFieldProtoBufOrdinary<a_##FieldName,EmbeddedUnitField<a_##FieldName,Type,Id>,Id,RepeatedTraits<Type>>; \
        using ___##FieldName=RepeatedFieldProtoBufOrdinary<a_##FieldName,SharedUnitField<a_##FieldName,Type,Id>,IdRepeatedTraits<Type>>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,___##FieldName,RepeatedTraits<Type>,Id)

#define _HDU_FIELD_REPEATED_DATAUNIT_PROTOBUF(FieldName,Type,Id) \
        _HDU_FIELD_REPEATED_DATAUNIT_PROTOBUF_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_REPEATED_DATAUNIT_PROTOBUF_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_REPEATED_DATAUNIT_PROTOBUF_WITH_DESCRIPTION, \
                _HDU_FIELD_REPEATED_DATAUNIT_PROTOBUF \
                ))
#define _HDU_FIELD_REPEATED_DATAUNIT_PROTOBUF_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_REPEATED_DATAUNIT_PROTOBUF_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare a repeated external dataunit field compatible with repeated unpacked type of Google Protocol Buffers
#define _HDU_FIELD_REPEATED_EXTERNAL_PROTOBUF_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        _HDU_FIELD_IS_CUSTOM_TYPE(Type) \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=RepeatedFieldProtoBufOrdinary<a_##FieldName,SharedUnitFieldTmpl<Type>,Id,RepeatedTraits<Type>>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,RepeatedTraits<Type>,Id)

#define _HDU_FIELD_REPEATED_EXTERNAL_PROTOBUF(FieldName,Type,Id) \
        _HDU_FIELD_REPEATED_EXTERNAL_PROTOBUF_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_REPEATED_EXTERNAL_PROTOBUF_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_REPEATED_EXTERNAL_PROTOBUF_WITH_DESCRIPTION, \
                _HDU_FIELD_REPEATED_EXTERNAL_PROTOBUF \
                ))
#define _HDU_FIELD_REPEATED_EXTERNAL_PROTOBUF_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_REPEATED_EXTERNAL_PROTOBUF_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare a repeated embedded dataunit field compatible with repeated unpacked type of Google Protocol Buffers
#define _HDU_FIELD_REPEATED_EMBEDDED_PROTOBUF_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
        _HDU_FIELD_IS_CUSTOM_TYPE(Type) \
        _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
        using a__##FieldName=RepeatedFieldProtoBufOrdinary<a_##FieldName,EmbeddedUnitFieldTmpl<Type>,Id,RepeatedTraits<Type>>; \
        _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,RepeatedTraits<Type>,Id)

#define _HDU_FIELD_REPEATED_EMBEDDED_PROTOBUF(FieldName,Type,Id) \
        _HDU_FIELD_REPEATED_EMBEDDED_PROTOBUF_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_REPEATED_EMBEDDED_PROTOBUF_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_REPEATED_EMBEDDED_PROTOBUF_WITH_DESCRIPTION, \
                _HDU_FIELD_REPEATED_EMBEDDED_PROTOBUF \
                ))
#define _HDU_FIELD_REPEATED_EMBEDDED_PROTOBUF_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_REPEATED_EMBEDDED_PROTOBUF_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


//! Use this macro to declare a repeated dataunit field compatible with repeated packed type of Google Protocol Buffers
#define _HDU_FIELD_REPEATED_PROTOBUF_PACKED_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
    static_assert(CheckBasicType<Type>::ok(),"Invalid type of packed repeated field"); \
    _HDU_FIELD_BEGIN(FieldName,Type,Id,Description) \
    using a__##FieldName=RepeatedFieldProtoBufPacked<a_##FieldName,Type,Id,RepeatedTraits<Type>>; \
    _HDU_FIELD_END(FieldName,a__##FieldName,a__##FieldName,RepeatedTraits<Type>,Id)

#define _HDU_FIELD_REPEATED_PROTOBUF_PACKED(FieldName,Type,Id) \
        _HDU_FIELD_REPEATED_PROTOBUF_PACKED_WITH_DESCRIPTION(FieldName,Type,Id,"")

#define _HDU_FIELD_REPEATED_PROTOBUF_PACKED_MACRO_CHOOSER(...) \
    _DSC_DU_EXPAND(_HDU_FIELD_DEF_GET_ARG5(__VA_ARGS__, \
                _HDU_FIELD_REPEATED_PROTOBUF_PACKED_WITH_DESCRIPTION, \
                _HDU_FIELD_REPEATED_PROTOBUF_PACKED \
                ))
#define _HDU_FIELD_REPEATED_PROTOBUF_PACKED_DEF(...) _DSC_DU_EXPAND(_HDU_FIELD_REPEATED_PROTOBUF_PACKED_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))


#define _HDU_INSTANTIATE_DATAUNIT(UnitName) \
    HATN_IGNORE_UNUSED_FUNCTION_BEGIN \
    namespace UnitName { \
        _HDU_DATAUNIT_IMPLEMENT_TYPE_IMPL_(type_impl,_type_impl) \
        _HDU_DATAUNIT_IMPLEMENT_TYPE_IMPL_(shared_fields_type,_shared_fields_type) \
    } \
    HATN_IGNORE_UNUSED_FUNCTION_END

//! Instantiate DataUnit extension class
#define _HDU_INSTANTIATE_DATAUNIT_EXT(UnitName,ExtensionName) \
    namespace UnitName { \
        namespace _Ext##ExtensionName \
        { \
            _HDU_DATAUNIT_IMPLEMENT_TYPE_IMPL_(type_impl,_type_impl) \
            _HDU_DATAUNIT_IMPLEMENT_TYPE_IMPL_(shared_fields_type,_shared_fields_type) \
        } \
        HATN_INC_COUNT(ExtensionsInst) \
    }

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITMACROS_H
