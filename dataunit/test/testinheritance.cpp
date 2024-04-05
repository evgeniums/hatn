#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/unitmacros.h>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>

using namespace hatn::dataunit;
using namespace hatn::dataunit::types;
using namespace hatn::dataunit::meta;

namespace {

HDU_V2_UNIT(def1,
    HDU_V2_DEFAULT_FIELD(f30,TYPE_INT32,30,10)
    HDU_V2_ENUM(e1,One,Two,Three)
    HDU_V2_DEFAULT_FIELD(f90,HDU_V2_TYPE_ENUM(e1),90,e1::Three)
)

HDU_V2_DEFAULT_PREPARE(f90,def1::e1,def1::e1::Three)

HDU_V2_UNIT(b1,
    HDU_V2_FIELD(f1,TYPE_BOOL,1)
)

HDU_V2_UNIT_WITH(ext1,(HDU_V2_BASE(b1)),
    HDU_V2_FIELD(f2,TYPE_BOOL,2)
)

struct type1 : public hana::true_{};
struct type2 : public hana::false_{};

constexpr auto m1=hana::make_map(
    hana::make_pair(hana::type_c<type1>, hana::int_c<0>),
    hana::make_pair(hana::type_c<type2>, hana::int_c<1>)
    );

template <typename ...Types>
struct WithTuple
{
    template <typename T>
    auto field(T&&) -> decltype(auto)
    {
        using t=std::decay_t<T>;
        constexpr auto idx=hana::at_key(m1,hana::type_c<t>);
        return hana::at(vals,idx);
    }

    hana::tuple<Types...> vals;
};

template <typename MapT>
struct WithMap
{
    template <typename T>
    auto field(T&&) -> decltype(auto)
    {
        using t=std::decay_t<T>;
        return hana::at_key(m,hana::type_c<t>);
    }

    MapT m;
};

auto m2=hana::make_map(
        hana::make_pair(hana::type_c<type1>, int{0}),
        hana::make_pair(hana::type_c<type2>, std::string{})
    );
using WithM2=WithMap<decltype(m2)>;

template <typename ...Fields>
constexpr auto index_map=make_index_map(hana::tuple_t<Fields...>);

template <typename ...Fields>
struct WithIndexMap
{
    template <typename T>
    auto field(T&&) -> decltype(auto)
    {
        using t=std::decay_t<T>;
        return hana::at(fields,hana::at_key(index_map<Fields...>,hana::type_c<t>));
    }

    hana::tuple<Fields...> fields;
};


template <typename ConfT, typename MapT>
struct ConfWithMap : public ConfT
{
    constexpr static MapT map{};
};

}

BOOST_AUTO_TEST_SUITE(TestInherit)

BOOST_AUTO_TEST_CASE(InheritanceMetaV2)
{
    static_assert(hana::value(hana::equal(hana::tuple_t<int, bool>,hana::make_tuple(hana::type_c<int>,hana::type_c<bool>))),"");

    WithTuple<type1,type2> t1;

    auto& v1=t1.field(type1{});
    static_assert(std::decay_t<decltype(v1)>::value,"");
    auto& v2=t1.field(type2{});
    static_assert(!std::decay_t<decltype(v2)>::value,"");
    BOOST_CHECK(true);

    WithM2 t2;
    auto& v3=t2.field(type1{});
    static_assert(std::is_same<int,std::decay_t<decltype(v3)>>::value,"");
    auto& v4=t2.field(type2{});
    static_assert(std::is_same<std::string,std::decay_t<decltype(v4)>>::value,"");
    BOOST_CHECK(true);

    auto idxMap=make_index_map(hana::tuple_t<type1,type2>);
    std::ignore=idxMap;

    WithIndexMap<type1,type2> t3;
    auto& v5=t3.field(type1{});
    static_assert(std::is_same<type1,std::decay_t<decltype(v5)>>::value,"");
    auto& v6=t3.field(type2{});
    static_assert(std::is_same<type2,std::decay_t<decltype(v6)>>::value,"");
    BOOST_CHECK(true);

    using mapT=decltype(index_map<type1,type2>);
    using cwm=ConfWithMap<hana::true_,mapT>;
    cwm t4;
    std::ignore=t4;
}

BOOST_AUTO_TEST_CASE(InheritObjV2)
{
    b1::type v1;
    const auto& fields1=b1::fields;

    auto m1=b1::type::conf::fields_map;
    std::ignore=m1;

    auto& f1=v1.field(fields1.f1);
    BOOST_CHECK(!f1.isSet());
    BOOST_CHECK(!f1.value());

    ext1::type v2;
    const auto& fields2=ext1::fields;

    auto& f2=v2.field(fields2.f2);
    BOOST_CHECK(!f2.isSet());
    BOOST_CHECK(!f2.value());

    auto& f2_1=v2.field(fields1.f1);
    BOOST_CHECK(!f2_1.isSet());
    BOOST_CHECK(!f2_1.value());

    f2_1.set(true);
    BOOST_CHECK(f2_1.isSet());
    BOOST_CHECK(f2_1.value());

    BOOST_CHECK(!f2.isSet());
    BOOST_CHECK(!f2.value());
}

BOOST_AUTO_TEST_SUITE_END()
