#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/unitmacros.h>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>

using namespace hatn::dataunit;
using namespace hatn::dataunit::types;
using namespace hatn::dataunit::meta;

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
        using hana_tag=FieldTag;

        using traits=meta::field_traits<meta::field_generator,
                                  field0_strings,
                                  hana::int_<field0_id>,
                                  hana::int_<HATN_COUNTER_GET(c)>,
                                  field0_type,
                                  field0_default_traits,
                                  field0_required,
                                  field0_repeated_traits
                                 >;

        using type=typename traits::type;
        using shared_type=typename traits::shared_type;
    };

    HATN_COUNTER_INC(c);
}

using field0=unit1::field<0>;

HDU_V2_UNIT(du1,
    HDU_V2_OPTIONAL_FIELD_WITH_DESCRIPTION(f10,TYPE_INT32,10,"Optional field 10")
    HDU_V2_REQUIRED_FIELD_WITH_DESCRIPTION(f20,TYPE_INT64,20,"Required field 20")
    HDU_V2_DEFAULT_FIELD_WITH_DESCRIPTION(f30,TYPE_DOUBLE,30,"Required field 30",10.30)
    HDU_V2_REPEATED_FIELD_NORMAL_WITH_DESCRIPTION(f40,TYPE_UINT16,40,"Repeated field 40",false,2312)
    HDU_V2_REPEATED_FIELD_PBPACKED_WITH_DESCRIPTION(f50,TYPE_UINT16,50,"Repeated protobuf packed field 50",true,1122)
    HDU_V2_REPEATED_FIELD_PBORDINARY_WITH_DESCRIPTION(f60,TYPE_UINT16,60,"Repeated protobuf ordinary field 60",false,3344)
    HDU_V2_UNIT_FIELD_WITH_DESCRIPTION(f70,TYPE_DATAUNIT,70,"Dataunit field 70",true)
    HDU_V2_REPEATED_UNIT_FIELD_WITH_DESCRIPTION(f80,TYPE_DATAUNIT,80,"Dataunit field 80",true)
    HDU_V2_ENUM(e1,One,Two,Three)
    HDU_V2_DEFAULT_FIELD_WITH_DESCRIPTION(f90,HDU_V2_TYPE_ENUM(e1),90,"Enum field 90",e1::One)
    HDU_V2_OPTIONAL_FIELD_WITH_DESCRIPTION(f100,HDU_V2_TYPE_FIXED_STRING(64),100,"Fixed string field 100")
)

HDU_V2_UNIT(du2,
    HDU_V2_UNIT_FIELD_WITH_DESCRIPTION(f10,du1::TYPE,10,"Subunit field 10",true)
    HDU_V2_REPEATED_UNIT_FIELD_WITH_DESCRIPTION(f11,du1::TYPE,11,"Repeated field 11",true)
    HDU_V2_REPEATED_EXTERNAL_UNIT_FIELD_WITH_DESCRIPTION(f12,du1::TYPE,12,"External field 12",true)
    HDU_V2_REPEATED_EMBEDDED_UNIT_FIELD_WITH_DESCRIPTION(f13,du1::TYPE,13,"Embedded field 13",true)
    HDU_V2_REPEATED_UNIT_FIELD_PBORDINARY_WITH_DESCRIPTION(f17,du1::TYPE,17,"Repeated pb ordinary field 17",true)
    HDU_V2_REPEATED_EXTERNAL_UNIT_FIELD_PBORDINARY_WITH_DESCRIPTION(f18,du1::TYPE,18,"External pb ordinary field 18",true)
    HDU_V2_REPEATED_EMBEDDED_UNIT_FIELD_PBORDINARY_WITH_DESCRIPTION(f19,du1::TYPE,19,"Embedded pb ordinary field 19",true)
)

HDU_V2_UNIT(du3,
    HDU_V2_OPTIONAL_FIELD_WITH_DESCRIPTION(f10,TYPE_INT32,10,"Optional field 10")
)

HDU_V2_UNIT(du4,
    HDU_V2_REQUIRED_FIELD_WITH_DESCRIPTION(f20,TYPE_INT64,20,"Required field 20")
)

HDU_V2_UNIT_WITH(du5,
    (HDU_V2_BASE(du3), HDU_V2_BASE(du4)),
    HDU_V2_OPTIONAL_FIELD_WITH_DESCRIPTION(f200,TYPE_INT32,200,"Optional field 200")
)

HDU_V2_UNIT_WITH(du6,
    (HDU_V2_BASE(du5)),
    HDU_V2_OPTIONAL_FIELD_WITH_DESCRIPTION(f300,TYPE_INT32,300,"Optional field 300")
)

HDU_V2_UNIT_EMPTY(due)

HDU_V2_UNIT_WITH(du7,
                 (HDU_V2_BASE(due)),
                 HDU_V2_OPTIONAL_FIELD_WITH_DESCRIPTION(f300,TYPE_INT32,300,"Optional field 300")
                 )

using f10=du1::field<0>;
using f20=du1::field<1>;
using f30=du1::field<2>;

namespace {
template <int Id>
struct WithId
{
    constexpr static auto id=hana::int_c<Id>;
};

template <typename T>
struct WithName
{
    constexpr static auto name=T{};
};
}

BOOST_AUTO_TEST_SUITE(TestMeta)

BOOST_AUTO_TEST_CASE(SimpleField)
{
    field0::type f0(nullptr);
}

BOOST_AUTO_TEST_CASE(MacroV2Declare)
{
    f10::type f10(nullptr);
    f20::type f20(nullptr);
    f30::type f30(nullptr);

    // const auto& fields=du1::traits::fields;
    // std::ignore=fields.f10;

    du1::type* vduP1=nullptr;
    du1::shared_type* vduP2=nullptr;
    BOOST_CHECK(vduP1==nullptr);
    BOOST_CHECK(vduP2==nullptr);

    du1::type vdu1;
    du1::shared_type vdu2;

    du2::type* vduP3=nullptr;
    du2::shared_type* vduP4=nullptr;
    BOOST_CHECK(vduP3==nullptr);
    BOOST_CHECK(vduP4==nullptr);

    du2::type vdu3;
    du2::shared_type vdu4;

    du5::type vdu5;
    auto& vf10=vdu5.field(du3::f10);
    std::ignore=vf10;

    du6::type vdu6;

    static_assert(decltype(meta::is_unit_type<TYPE_DATAUNIT>())::value,"");
    static_assert(!decltype(meta::is_unit_type<uint32_t>())::value,"");
    static_assert(decltype(meta::is_unit_type<du1::TYPE>())::value,"");
    static_assert(!decltype(meta::is_unit_type<TYPE_BYTES>())::value,"");
    static_assert(decltype(meta::is_basic_type<TYPE_BYTES>())::value,"");
    static_assert(!decltype(meta::is_basic_type<uint32_t>())::value,"");

    static_assert(decltype(meta::check_ids_unique(du1::field_defs))::value,"");
    constexpr auto f2=hana::append(du1::field_defs,hana::type_c<WithId<30>>);
    static_assert(!decltype(meta::check_ids_unique(f2))::value,"");

#ifdef HATN_STRING_LITERAL
    static_assert(decltype(check_names_unique(du1::field_defs))::value,"");
    constexpr auto f20n="f20"_s;
    constexpr auto f3=hana::append(du1::field_defs,hana::type_c<WithName<decltype(f20n)>>);
    static_assert(!decltype(check_names_unique(f3))::value,"");
#else
    static_assert(decltype(check_names_unique(du1::field_defs))::value,"");
#endif

    due::type emptyUnit;

    du7::type emptyUnitDerived;
}

BOOST_AUTO_TEST_SUITE_END()
