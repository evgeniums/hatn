#include <boost/test/unit_test.hpp>

#include <boost/hana.hpp>
namespace hana=boost::hana;

#include <hatn/dataunit/unitmacros.h>

using namespace hatn::dataunit;
using namespace hatn::dataunit::types;

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
        using shared_type=typename traits::shared_type;
    };

    HATN_COUNTER_INC(c);
}

using field0=unit1::field<0>;

HDU_V2_DATAUNIT(du1,
    HDU_V2_OPTIONAL_FIELD_WITH_DESCRIPTION(f10,TYPE_INT32,10,"Optional field 10")
    HDU_V2_REQUIRED_FIELD_WITH_DESCRIPTION(f20,TYPE_INT64,20,"Required field 20")
    HDU_V2_DEFAULT_FIELD_WITH_DESCRIPTION(f30,TYPE_DOUBLE,30,"Required field 30",10.30)
    HDU_V2_REPEATED_FIELD_NORMAL_WITH_DESCRIPTION(f40,TYPE_UINT16,40,"Repeated field 40",false,2312)
    HDU_V2_REPEATED_FIELD_PBPACKED_WITH_DESCRIPTION(f50,TYPE_UINT16,50,"Repeated protobuf packed field 50",true,1122)
    HDU_V2_REPEATED_FIELD_PBORDINARY_WITH_DESCRIPTION(f60,TYPE_UINT16,60,"Repeated protobuf ordinary field 60",false,3344)
    HDU_V2_DATAUNIT_FIELD_WITH_DESCRIPTION(f70,TYPE_DATAUNIT,70,"Dataunit field 70",true)
)

HDU_V2_DATAUNIT(du2,
    HDU_V2_DATAUNIT_FIELD_WITH_DESCRIPTION(f10,du1::TYPE,10,"Subunit field 10",true)
)

using f10=du1::field<0>;
using f20=du1::field<1>;
using f30=du1::field<2>;

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

    auto fields=du1::fields;

    du1::type* vdu1=nullptr;
    du1::shared_type* vdu2=nullptr;
    BOOST_CHECK(vdu1==nullptr);
    BOOST_CHECK(vdu2==nullptr);

    du2::type* vdu3=nullptr;
    du2::shared_type* vdu4=nullptr;
    BOOST_CHECK(vdu3==nullptr);
    BOOST_CHECK(vdu4==nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
