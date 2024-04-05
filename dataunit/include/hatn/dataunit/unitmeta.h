/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/unitmeta.h
  *
  *  Contains meta function and types for data unit generation.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITMETA_H
#define HATNDATAUNITMETA_H

#include <utility>

#include <boost/hana.hpp>
namespace hana=boost::hana;

#include <hatn/common/metautils.h>

#include <hatn/dataunit/dataunit.h>

#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/types.h>
#include <hatn/dataunit/unittraits.h>
#include <hatn/dataunit/fields/repeated.h>
#include <hatn/dataunit/allocatorfactory.h>

//! @todo Move it to configuration script. Test with msvc c++20, maybe it is already supported there.
#ifndef _MSC_VER
#ifndef HATN_STRING_LITERAL
#define HATN_STRING_LITERAL
#endif
#endif

HATN_DATAUNIT_NAMESPACE_BEGIN

namespace meta {

#ifdef HATN_STRING_LITERAL

HATN_IGNORE_STRING_LITERAL_BEGIN

template <typename CharT, CharT ...s>
constexpr auto operator"" _s() {
    return hana::string_c<s...>;
}

HATN_IGNORE_STRING_LITERAL_END

#endif

//---------------------------------------------------------------

struct field_id_tag{};
struct auto_tag{};

struct auto_t
{
    using hana_tag=auto_tag;
};
constexpr auto_t Auto{};

//---------------------------------------------------------------

template <typename T, typename ValueT>
struct default_t
{
    using hana_tag=DefaultValueTag; \
    static typename T::type value(){return static_cast<typename T::type>(ValueT::value);}
    using HasDefV=std::true_type;
};

template <typename T, typename ValueT, typename=hana::when<true>>
struct default_type : public auto_t
{
    static_assert(!T::isUnitType::value || hana::is_a<auto_tag,typename ValueT::type>,"Default value can not be used for data unit field");
    static_assert(!T::isBytesType::value || T::isStringType::value || hana::is_a<auto_tag,typename ValueT::type>,"Default value can not be used for bytes field");
};

template <typename T, typename ValueT>
struct default_type<T, ValueT, hana::when<
            !hana::is_a<auto_tag,typename ValueT::type>
            &&
            std::is_constructible<typename T::type, typename ValueT::type>::value
        >>
    : public default_t<T,ValueT>{};

template <typename T, typename ValueT>
struct default_type<T, ValueT, hana::when<
                                   T::isEnum::value
                                          &&
                                          std::is_constructible<typename T::Enum, typename ValueT::type>::value
                                   >>
    : public default_t<T,ValueT>{};

template <typename T, typename = hana::when<true>>
struct default_field
{
    template <typename Type>
    using type=T;
};

template <typename T>
struct default_field<T, hana::when<hana::is_a<auto_tag,T>>>
{
    template <typename Type>
    using type=DefaultValue<Type>;
};

//---------------------------------------------------------------

template <template <typename ...> class GeneratorT,
         typename StringsT,
         typename Id,
         typename Index,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedType
         >
struct field_traits
{
    using hana_tag=FieldTag;

    using default_traits=typename default_field<DefaultTraits>::template type<TypeId>;
    using generator=GeneratorT<StringsT,Id,TypeId,default_traits,Required,RepeatedType>;

    using Type=typename generator::type_id;
    using type=typename generator::type;
    using shared_type=typename generator::shared_type;

    constexpr static const int ID=Id::value;
    constexpr static const int index=Index::value;

    constexpr static const char* name() noexcept {return StringsT::name;}

    constexpr static int id() noexcept {return Id::value;}

    constexpr static bool required() noexcept {return Required::value;}

    template <typename T>
    bool operator ==(const T& other) const noexcept
    {
        return std::is_same<Type,typename T::Type>::value && id()==other.id();
    }

    bool operator !=(const field_traits& other) const noexcept
    {
        return !(*this==other);
    }
};

//---------------------------------------------------------------

struct repeated_tag{};

template <RepeatedMode Mode=RepeatedMode::Auto, RepeatedContentType ContentType=RepeatedContentType::Auto>
struct repeated_config
{
    using hana_tag=repeated_tag;

    constexpr static auto mode=Mode;
    constexpr static auto content=ContentType;
};

template <typename TypeId, RepeatedMode Mode, RepeatedContentType ContentType, typename = hana::when<true>>
struct repeated_traits
{
    using selector=SelectRepeatedType<Mode>;

    template <typename FieldName,typename Type,int Id,typename DefaultAlias,bool Required>
    using type=typename selector::template type<FieldName,Type,Id,RepeatedTraits<Type>,DefaultAlias,Required>;

    template <typename FieldName,typename Type,int Id,typename DefaultAlias,bool Required>
    using shared_type=type<FieldName,Type,Id,DefaultAlias,Required>;
};

template <typename TypeId, RepeatedMode Mode>
struct repeated_traits<TypeId,Mode,RepeatedContentType::Auto, hana::when<TypeId::isUnitType::value>>
{
    using selector=SelectRepeatedType<Mode>;

    template <typename FieldName,typename Type,int Id,typename DefaultAlias,bool Required>
    using type=typename selector::template type<FieldName,EmbeddedUnitFieldTmpl<Type>,Id,RepeatedTraits<Type>,DefaultAlias,Required>;

    template <typename FieldName,typename Type,int Id,typename DefaultAlias,bool Required>
    using shared_type=typename selector::template type<FieldName,SharedUnitFieldTmpl<Type>,Id,RepeatedTraits<Type>,DefaultAlias,Required>;
};

template <typename TypeId, RepeatedMode Mode>
struct repeated_traits<TypeId,Mode,RepeatedContentType::External, hana::when<TypeId::isUnitType::value>>
{
    using selector=SelectRepeatedType<Mode>;

    template <typename FieldName,typename Type,int Id,typename DefaultAlias,bool Required>
    using type=typename selector::template type<FieldName,SharedUnitFieldTmpl<Type>,Id,RepeatedTraits<Type>,DefaultAlias,Required>;

    template <typename FieldName,typename Type,int Id,typename DefaultAlias,bool Required>
    using shared_type=type<FieldName,Type,Id,DefaultAlias,Required>;
};

template <typename TypeId, RepeatedMode Mode>
struct repeated_traits<TypeId,Mode,RepeatedContentType::Embedded, hana::when<TypeId::isUnitType::value>>
{
    using selector=SelectRepeatedType<Mode>;

    template <typename FieldName,typename Type,int Id,typename DefaultAlias,bool Required>
    using type=typename selector::template type<FieldName,EmbeddedUnitFieldTmpl<Type>,Id,RepeatedTraits<Type>,DefaultAlias,Required>;

    template <typename FieldName,typename Type,int Id,typename DefaultAlias,bool Required>
    using shared_type=type<FieldName,Type,Id,DefaultAlias,Required>;
};

//---------------------------------------------------------------

/**
 * @brief Generator of field type.
 */
template <typename StringsT,
         typename Id,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedType,
         typename = hana::when<true>
         >
struct field_generator
{
    using type=OptionalField<StringsT,TypeId,Id::value>;
    using shared_type=type;
    using type_id=TypeId;
};

/**
 * @brief Generator of field type for required fields.
 */
template <typename StringsT,
         typename Id,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedType
         >
struct field_generator<StringsT,Id,TypeId,DefaultTraits,Required,RepeatedType,
                       hana::when<
                           Required::value
                            &&
                            !hana::is_a<repeated_tag,RepeatedType>
                            &&
                           !TypeId::isUnitType::value
                           >
                       >
{
    using type=RequiredField<StringsT,TypeId,Id::value>;
    using shared_type=type;
    using type_id=TypeId;
};

/**
 * @brief Generator of field type for default fields.
 */
template <typename StringsT,
         typename Id,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedType
         >
struct field_generator<StringsT,Id,TypeId,DefaultTraits,Required,RepeatedType,
                       hana::when<
                               DefaultTraits::HasDefV::value
                               &&
                               !hana::is_a<repeated_tag,RepeatedType>
                           >
                       >
{
    //! @todo implement default string
    using type=DefaultField<StringsT,TypeId,Id::value,DefaultTraits>;
    using shared_type=type;
    using type_id=TypeId;
};

/**
 * @brief Generator of field type for repeated fields except for TYPE_DATAUNIT.
 */
template <typename StringsT,
         typename Id,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedType
         >
struct field_generator<StringsT,Id,TypeId,DefaultTraits,Required,RepeatedType,
                       hana::when<
                           hana::is_a<repeated_tag,RepeatedType>
                            &&
                           !std::is_same<TypeId,TYPE_DATAUNIT>::value
                           >
                       >
{
    static_assert(!TypeId::isUnitType::value || RepeatedType::mode!=RepeatedMode::ProtobufPacked,
                  "Protobuf packed mode can not be used for data unit fields");

    using traits=repeated_traits<TypeId,RepeatedType::mode,RepeatedType::content>;

    using type_id=RepeatedTraits<TypeId>;
    using type=typename traits::template type<StringsT,TypeId,Id::value,DefaultTraits,Required::value>;
    using shared_type=typename traits::template shared_type<StringsT,TypeId,Id::value,DefaultTraits,Required::value>;
};

/**
 * @brief Generator of field type for TYPE_DATAUNIT repeated fields.
 */
template <typename StringsT,
         typename Id,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedType
         >
struct field_generator<StringsT,Id,TypeId,DefaultTraits,Required,RepeatedType,
                       hana::when<
                           hana::is_a<repeated_tag,RepeatedType>
                            &&
                           std::is_same<TypeId,TYPE_DATAUNIT>::value
                           >
                       >
{
    static_assert(RepeatedType::content != RepeatedContentType::Embedded, "TYPE_DATAUNIT can not be Embedded, only either Auto or External");
    static_assert(RepeatedType::mode != RepeatedMode::ProtobufPacked,"Protobuf packed mode can not be used for data unit fields");

    using traits=repeated_traits<TypeId,RepeatedType::mode,RepeatedContentType::External>;

    using type_id=RepeatedTraits<TypeId>;
    using type=typename traits::template type<StringsT,TypeId,Id::value,DefaultTraits,Required::value>;
    using shared_type=typename traits::template shared_type<StringsT,TypeId,Id::value,DefaultValue<TypeId>,Required::value>;
};

/**
 * @brief Generator of field type for dataunit fields except for TYPE_DATAUNIT.
 */
template <typename StringsT,
         typename Id,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedType
         >
struct field_generator<StringsT,Id,TypeId,DefaultTraits,Required,RepeatedType,
                       hana::when<
                                  TypeId::isUnitType::value
                                  &&
                                  !std::is_same<TypeId,TYPE_DATAUNIT>::value
                                  &&
                                  !hana::is_a<repeated_tag,RepeatedType>
                           >
                       >
{
    using type_id=TypeId;
    using type=EmbeddedUnitField<StringsT,TypeId,Id::value,Required::value>;
    using shared_type=SharedUnitField<StringsT,TypeId,Id::value,Required::value>;
};

/**
 * @brief Generator of field type for TYPE_DATAUNIT fields.
 */
template <typename StringsT,
         typename Id,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedType
         >
struct field_generator<StringsT,Id,TypeId,DefaultTraits,Required,RepeatedType,
                       hana::when<
                           TypeId::isUnitType::value
                           &&
                           std::is_same<TypeId,TYPE_DATAUNIT>::value
                           &&
                           !hana::is_a<repeated_tag,RepeatedType>
                           >
                       >
{
    using type=SharedUnitField<StringsT,TypeId,Id::value,Required::value>;
    using shared_type=SharedUnitField<StringsT,TypeId,Id::value,Required::value>;
    using type_id=TypeId;
};

//---------------------------------------------------------------

template <template <int> class FieldT, int N>
struct make_fields_tuple_t
{
    constexpr auto operator()() const
    {
        constexpr std::make_integer_sequence<int,N> indices{};
        auto to_field_c=[](auto x)
        {
            return hana::type_c<FieldT<decltype(x)::value>>;
        };

        return hana::transform(hana::unpack(indices,
                                            [](auto ...i){return hana::make_tuple(std::forward<decltype(i)>(i)...);}
                                            ),
                               to_field_c
                               );
    }
};
template <template <int> class FieldT, int N>
constexpr make_fields_tuple_t<FieldT,N> make_fields_tuple{};

//---------------------------------------------------------------

struct check_ids_unique_t
{
    template <typename T>
    auto operator()(T fields) const
    {
        auto extract_id=[](auto x)
        {
            using type=typename decltype(x)::type;
            return type::id;
        };

        return hana::compose(
            hana::partial(hana::equal,hana::size(fields)),
            hana::size,
            hana::to_tuple,
            hana::to_set,
            hana::reverse_partial(hana::transform,extract_id)
        )(fields);
    }
};
constexpr check_ids_unique_t check_ids_unique{};

struct check_names_unique_t
{
#ifdef HATN_STRING_LITERAL

    template <typename T>
    auto operator()(T fields) const
    {
        auto extract_name=[](auto x)
        {
            using type=typename decltype(x)::type;
            return type::name;
        };

        return hana::compose(
            hana::partial(hana::equal,hana::size(fields)),
            hana::size,
            hana::to_tuple,
            hana::to_set,
            hana::reverse_partial(hana::transform,extract_name)
            )(fields);
    }

#else

    template <typename T>
    auto operator()(T) const
    {
        //! @note Not supported.
        return hana::true_c;
    }

#endif

};
constexpr check_names_unique_t check_names_unique{};

//---------------------------------------------------------------

template <typename ConfT>
struct unit
{
    template <typename ...Fields>
    using unit_t=DataUnit<ConfT,Fields...>;

    template <typename FieldsT>
    constexpr static auto type_c(FieldsT fields)
    {
        auto to_field_c=[](auto x)
        {
            using field_c=typename decltype(x)::type;
            using field_type=typename field_c::type;
            return hana::type_c<field_type>;
        };

        auto fields_c=hana::transform(fields,to_field_c);
        auto unit_c=hana::unpack(fields_c,hana::template_<unit_t>);
        return unit_c;
    }

    template <typename FieldsT>
    constexpr static auto shared_type_c(FieldsT fields)
    {
        auto to_field_c=[](auto x)
        {
            using field_c=typename decltype(x)::type;
            using field_type=typename field_c::shared_type;
            return hana::type_c<field_type>;
        };

        auto fields_c=hana::transform(fields,to_field_c);
        auto unit_c=hana::unpack(fields_c,hana::template_<unit_t>);
        return unit_c;
    }
};

template <typename UnitT>
class managed_unit : public ManagedUnit<UnitT>,
                     public common::WithStaticAllocator<managed_unit<UnitT>>
{
    public:
        using ManagedUnit<UnitT>::ManagedUnit;
};

template <typename SharedUnitT>
class shared_managed_unit : public ManagedUnit<SharedUnitT>,
                            public common::WithStaticAllocator<shared_managed_unit<SharedUnitT>>
{
    public:
        using ManagedUnit<SharedUnitT>::ManagedUnit;
};

//---------------------------------------------------------------

template <typename BaseT, typename UniqueType=void>
class unit_t : public BaseT
{
        public:

            unit_t(AllocatorFactory* factory=AllocatorFactory::getDefault());

            virtual const Field* fieldById(int id) const override;
            virtual Field* fieldById(int id) override;
            virtual bool iterateFields(const Unit::FieldVisitor& visitor) override;
            virtual bool iterateFieldsConst(const Unit::FieldVisitorConst& visitor) const override;
            virtual size_t fieldCount() const noexcept override;
            virtual const char* name() const noexcept override;
            inline const unit_t& value() const noexcept
            {
                return *this;
            }
            inline unit_t* mutableValue() noexcept
            {
                return this;
            }
            inline unit_t* castToUnit(Unit* unit) const noexcept
            {
                return common::dynamicCastWithSample(unit,this);
            }
            inline const unit_t* castToUnit(const Unit* unit) const noexcept
            {
                return common::dynamicCastWithSample(unit,this);
            }
            virtual int serialize(WireData& wired,bool topLevel=true) const override;
#if 0
            //! @todo Maybe implement virtual serialization
            virtual int serialize(WireBufSolid& wired,bool topLevel=true) const override;
            virtual int serialize(WireBufSolidShared& wired,bool topLevel=true) const override;
            virtual int serialize(WireBufChained& wired,bool topLevel=true) const override;
#endif
            virtual bool parse(WireData& wired,bool topLevel=true) override;
            virtual bool parse(WireBufSolid& wired,bool topLevel=true) override;
};

//---------------------------------------------------------------

template <typename FieldsT>
constexpr FieldsT field_ids_instance{};

template <typename UnitT, typename ManagedT, typename FieldsT>
struct unit_traits
{
    constexpr static const auto& fields=field_ids_instance<FieldsT>;

    using type=UnitT;
    using managed=ManagedT;
};

template <typename traits, typename shared_traits>
struct subunit : public types::TYPE_DATAUNIT
{
        using type=typename traits::type;
        using shared_type=common::SharedPtr<typename shared_traits::managed>;
        using base_shared_type=typename shared_traits::type;
        using Hatn=std::true_type;

        constexpr static const bool isSizeIterateNeeded=true;

        template <typename ...Args>
        static shared_type createManagedObject(AllocatorFactory* factory, Unit* unitBase)
        {
            if (factory==nullptr) factory=unitBase->factory();
            auto m=factory->createObject<typename shared_traits::managed>(factory);
            return m;
        }
};

//---------------------------------------------------------------

struct concat_fields_t
{
    template <typename ...Args>
    constexpr auto operator() (Args&& ...args) const
    {
        return hana::concat(std::forward<Args>(args)...);
    }

    template <typename Arg>
    constexpr auto operator() (Arg&& arg) const
    {
        return hana::concat(hana::make_tuple(),std::forward<Arg>(arg));
    }

    constexpr auto operator() () const
    {
        return hana::make_tuple();
    }
};
constexpr concat_fields_t concat_fields{};

//---------------------------------------------------------------

template <typename Type>
constexpr auto is_unit_type()
{
    auto check=[](auto x) -> decltype(hana::bool_c<decltype(x)::type::isUnitType::value>)
    {
        return hana::bool_c<decltype(x)::type::isUnitType::value>;
    };

    auto ok=hana::sfinae(check)(hana::type_c<Type>);
    return hana::equal(ok,hana::just(hana::true_c));
}

template <typename Type>
constexpr auto is_basic_type()
{
    auto check=[](auto x) -> decltype(hana::bool_c<decltype(x)::type::BasicType::value>)
    {
        return hana::bool_c<decltype(x)::type::BasicType::value>;
    };

    auto ok=hana::sfinae(check)(hana::type_c<Type>);
    return hana::equal(ok,hana::just(hana::true_c));
}

//---------------------------------------------------------------
#if 0
#define HDU_V2_TAG(TagName,Value) \
    struct field_tags_##FieldName<HATN_COUNTER_GET(t_c_##FieldName)>\
    {\
        constexpr static const char* name=#TagName;\
        constexpr static const auto value=Value;\
    };\
    constexpr field_tags_##FieldName<HATN_COUNTER_GET(t_c_##FieldName)> tags_##FieldName##TagName;

#define HDU_V2_TAGS(...) \
    hana::type_c<decltype(hana::make_tuple(__VA_ARGS__))>

#define HDU_V2_FIELD_TAGS(FieldName,...) \
    struct t_c_##FieldName{};\
    HATN_COUNTER_MAKE(t_c_##FieldName);\
    template <int> struct field_tags_##FieldName{};\
    __VA_ARGS__ \
    auto tags_##FieldName=lift_all_tags<field_tags_##FieldName,HATN_COUNTER_GET(t_c_##FieldName)>();
    template <>\
    struct field_tags<HATN_COUNTER_GET(t_c)>\
    {\
        constexpr static const auto& field=FieldName;\
        constexpr static const auto& value=tags_##FieldName;\
    };\

#define HDU_V2_UNIT_TAGS(UnitName,...) \
    struct t_c{};\
    HATN_COUNTER_MAKE(t_c);\
    template <int> struct field_tags{};\
    __VA_ARGS__ \
    auto tags=make_tags(field_tags);
#endif

//---------------------------------------------------------------
} // namespace meta

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITMETA_H
