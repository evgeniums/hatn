#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/unitmacros.h>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

using namespace hatn::dataunit;
using namespace hatn::dataunit::types;
using namespace hatn::dataunit::meta;

#if 1
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
    struct field0_strings
    {
        constexpr static const char* name=field0_name;
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
    HDU_V2_OPTIONAL_FIELD(f10,TYPE_INT32,10)
    HDU_V2_REQUIRED_FIELD(f20,TYPE_INT64,20)
    HDU_V2_DEFAULT_FIELD(f30,TYPE_DOUBLE,30,10.30)
    HDU_V2_REPEATED_BASIC_FIELD(f40,TYPE_UINT16,40,false,2312)
    HDU_V2_REQUIRED_FIELD(f70,TYPE_DATAUNIT,70)
    HDU_V2_REPEATED_UNIT_FIELD(f80,TYPE_DATAUNIT,80,true)
    HDU_V2_ENUM(e1,One,Two,Three)
    HDU_V2_DEFAULT_FIELD(f90,HDU_V2_TYPE_ENUM(e1),90,e1::One)
    HDU_V2_OPTIONAL_FIELD(f100,HDU_V2_TYPE_FIXED_STRING(64),100)
)

HDU_V2_UNIT(du2,
    HDU_V2_REQUIRED_FIELD(f10,du1::TYPE,10)
    HDU_V2_REPEATED_UNIT_FIELD(f11,du1::TYPE,11,true)
    HDU_V2_REPEATED_EXTERNAL_UNIT_FIELD(f12,du1::TYPE,12,true)
    HDU_V2_REPEATED_EMBEDDED_UNIT_FIELD(f13,du1::TYPE,13,false)
)

HDU_V2_UNIT(du3,
    HDU_V2_OPTIONAL_FIELD(f10,TYPE_INT32,10)
)

HDU_V2_UNIT(du4,
    HDU_V2_REQUIRED_FIELD(f20,TYPE_INT64,20)
)

HDU_V2_UNIT_WITH(du5,
    (HDU_V2_BASE(du3), HDU_V2_BASE(du4)),
    HDU_V2_OPTIONAL_FIELD(f200,TYPE_INT32,200)
)

HDU_V2_UNIT_WITH(du6,
    (HDU_V2_BASE(du5)),
    HDU_V2_OPTIONAL_FIELD(f300,TYPE_INT32,300)
)

HDU_V2_UNIT_EMPTY(due)

HDU_V2_UNIT(du_min,
    HDU_V2_REQUIRED_FIELD(f1,TYPE_BOOL,1)
)

HDU_V2_UNIT_WITH(du7,
    (HDU_V2_BASE(due)),
    HDU_V2_FIELD(f300,TYPE_INT32,300)
    HDU_V2_FIELD(f400,TYPE_UINT32,400,true)
    HDU_V2_FIELD(f500,TYPE_UINT64,500,false,5577)
    HDU_V2_FIELD(f510,TYPE_BYTES,510,false,Auto)
    HDU_V2_REPEATED_FIELD(f600,TYPE_INT32,600)
    HDU_V2_REPEATED_FIELD(f700,TYPE_UINT32,700,true)
    HDU_V2_REPEATED_FIELD(f800,TYPE_UINT64,800,false,8899)
    HDU_V2_REPEATED_FIELD(f810,TYPE_UINT64,810,false,Auto,ProtobufPacked)
    HDU_V2_REPEATED_FIELD(f820,TYPE_UINT64,820,false,Auto,ProtobufOrdinary)
    HDU_V2_REPEATED_FIELD(f900,du_min::TYPE,900)
    HDU_V2_REPEATED_FIELD(f1000,du_min::TYPE,1000,true)
    HDU_V2_REPEATED_FIELD(f1100,du_min::TYPE,1100)
    HDU_V2_REPEATED_FIELD(f1200,du_min::TYPE,1200,false,Auto,Auto,External)
    HDU_V2_REPEATED_FIELD(f1300,du_min::TYPE,1300,true,Auto,Auto,Embedded)

    // HDU_V2_REPEATED_FIELD(f17,du_min::TYPE,17,true,Auto,ProtobufPacked)
    HDU_V2_REPEATED_FIELD(f18,TYPE_DATAUNIT,18,false,Auto,ProtobufOrdinary,External)
)

HDU_V2_UNIT(du8,
    HDU_V2_FIELD(f1,TYPE_BYTES,1)
    HDU_V2_FIELD(f2,TYPE_BYTES,2,true)

    HDU_V2_FIELD(f3,TYPE_STRING,3)
    HDU_V2_FIELD(f4,TYPE_STRING,4,true)
    HDU_V2_FIELD(f5,TYPE_STRING,5,false,"Hello world!")

    HDU_V2_FIELD(f6,HDU_V2_TYPE_FIXED_STRING(64),6)
    HDU_V2_FIELD(f7,HDU_V2_TYPE_FIXED_STRING(64),7,true)
    HDU_V2_FIELD(f8,HDU_V2_TYPE_FIXED_STRING(64),8,false,"Hi!")
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

    const auto& fields=du1::traits::fields;
    std::ignore=fields.f10;

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

    du8::type bytes;
}

BOOST_AUTO_TEST_SUITE_END()

#endif
