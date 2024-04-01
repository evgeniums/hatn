#include <boost/test/unit_test.hpp>

#include <utility>
#include <boost/hana.hpp>
namespace hana=boost::hana;

#include <hatn/common/metautils.h>
#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/types.h>

#include <hatn/dataunit/unittraits.h>

#if 1
using namespace hatn::dataunit;
using namespace hatn::dataunit::types;

struct field_tag{};
struct field_traits_tag{};

template <template <typename ...> class GeneratorT,
         typename StringsT,
         typename Id,
         typename Index,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedTraits
         >
struct field_traits
{
    using hana_tag=field_traits_tag;

    using type_id=TypeId;
    using default_traits=DefaultTraits;

    using type=typename GeneratorT<StringsT,Id,TypeId,DefaultTraits,Required,RepeatedTraits>::type;

    constexpr static const char* name() {return StringsT::name;}

    constexpr static const char* description() {return StringsT::description;}

    constexpr static int index() noexcept {return Index::value;}

    constexpr static int id() noexcept {return Id::value;}

    constexpr static bool required() noexcept {return Required::value;}
};

template <typename traits, typename=hana::when<true>>
struct shared_field
{
    using type=typename traits::type;
};

template <typename StringsT,
         typename Id,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedTraits,
         typename = hana::when<true>
         >
struct field_generator
{
    using type=OptionalField<StringsT,TypeId,Id::value>;
};

template <typename StringsT,
         typename Id,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedTraits
         >
struct field_generator<StringsT,Id,TypeId,DefaultTraits,Required,RepeatedTraits,
                        hana::when<
                           Required::value
                           >
                       >
{
    using type=RequiredField<StringsT,TypeId,Id::value>;
};

template <typename StringsT,
         typename Id,
         typename TypeId,
         typename DefaultTraits,
         typename Required,
         typename RepeatedTraits
         >
struct field_generator<StringsT,Id,TypeId,DefaultTraits,Required,RepeatedTraits,
                       hana::when<
                            hana::is_a<DefaultValueTag,DefaultTraits>
                            &&
                            !TypeId::isRepeatedType::value
                           >
                       >
{
    //! @todo implement default string
    using type=DefaultField<StringsT,TypeId,Id::value,DefaultTraits>;
};

template <template <int> class FieldT, int N>
struct make_fields_tuple_t
{
    auto operator()() const
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

namespace unit1 {

    struct c{};

    using namespace hatn::dataunit;
    using namespace hatn::dataunit::types;
    HATN_COUNTER_MAKE(c);
    template <int N> struct field{};

    using field0_type=TYPE_INT32;
    struct field0{};
    constexpr int field0_id=10;
    constexpr const char* field0_name="field0";
    constexpr const char* field0_description="description0";
    struct field0_strings
    {
        constexpr static const char* name=field0_name;
        constexpr static const char* description=field0_description;
    };

    using field0_default_traits=hana::false_;
    using field0_required=hana::false_;
    using field0_repeated_traits=hana::false_;

    template <>
    struct field<HATN_COUNTER_GET(c)>
    {
        using hana_tag=field_tag;

        using traits=field_traits<field_generator,
                                  field0_strings,
                                  hana::int_<field0_id>,
                                  hana::int_<HATN_COUNTER_GET(c)>,
                                  field0_type,
                                  field0_default_traits,
                                  field0_required,
                                  field0_repeated_traits
                                 >;

        using type=typename traits::type;
        using shared_type=typename shared_field<traits>::type;
    };

    HATN_COUNTER_INC(c);
}

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
        using shared_type=typename shared_field<traits>::type;\
    };\
    HATN_COUNTER_INC(c);

#define HDU_V2_DEFAULT_TRAITS(FieldName,Type,Default) \
    struct default_##FieldName\
    {\
        using hana_tag=DefaultValueTag; \
        static typename Type::type value(){return static_cast<typename Type::type>(Default);} \
        using HasDefV=std::true_type; \
    };

#define HDU_V2_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::false_,hana::false_)

#define HDU_V2_REQUIRED_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Description) \
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,hana::false_,hana::true_,hana::false_)

#define HDU_V2_DEFAULT_FIELD_WITH_DESCRIPTION(FieldName,Type,Id,Default,Description) \
    HDU_V2_DEFAULT_TRAITS(FieldName,Type,Default)\
    HDU_V2_FIELD_DEF(FieldName,Type,Id,Description,default_##FieldName,hana::false_,hana::false_)

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
    using type=decltype(unit_c)::type;\
    auto shared_unit_c=unit<conf>::shared_type_c(fields);\
    using shared_type=decltype(shared_unit_c)::type;\
}

using field0=unit1::field<0>;

HDU_V2_DATAUNIT(du1,
    HDU_V2_FIELD_WITH_DESCRIPTION(f10,TYPE_INT32,10,"Optional field 10")
    HDU_V2_REQUIRED_FIELD_WITH_DESCRIPTION(f20,TYPE_INT64,20,"Required field 20")
    HDU_V2_DEFAULT_FIELD_WITH_DESCRIPTION(f30,TYPE_DOUBLE,30,10.30,"Required field 30")
)

using f10=du1::field<0>;
using f20=du1::field<1>;
using f30=du1::field<2>;

BOOST_AUTO_TEST_SUITE(TestMeta)

BOOST_AUTO_TEST_CASE(SimpleField)
{
    field0::type f0(nullptr);
}

BOOST_AUTO_TEST_CASE(MacroFields)
{
    f10::type f10(nullptr);
    f20::type f20(nullptr);
    f30::type f30(nullptr);

    auto fields=du1::fields;

    du1::type* vdu1=nullptr;
    du1::shared_type* vdu2=nullptr;
    BOOST_CHECK(vdu1==nullptr);
    BOOST_CHECK(vdu2==nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
