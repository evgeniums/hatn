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

template <typename ...Types>
struct type_c_tuple
{};

template <typename Type, typename ...Types>
struct type_c_tuple<Type,Types...>
{
    constexpr static auto value=hana::append(type_c_tuple<Types...>::value,hana::type_c<Type>);
};

template <typename Type>
struct type_c_tuple<Type>
{
    constexpr static auto value=hana::make_tuple(hana::type_c<Type>);
};

struct make_index_map_t
{
    template <typename TypesC>
    constexpr auto operator()(TypesC typesC) const
    {
        constexpr auto indexes=hana::make_range(hana::int_c<0>,hana::size(typesC));
        constexpr auto pairs=hana::zip_with(hana::make_pair,hana::reverse(typesC),hana::unpack(indexes,hana::make_tuple));
        return hana::unpack(pairs,hana::make_map);
    }
};
constexpr make_index_map_t make_index_map{};

template <typename ...Fields>
struct WithIndexMap
{
    constexpr static auto m=make_index_map(type_c_tuple<Fields...>::value);

    template <typename T>
    auto field(T&&) -> decltype(auto)
    {
        using t=std::decay_t<T>;
        return hana::at(fields,hana::at_key(m,hana::type_c<t>));
    }

    hana::tuple<Fields...> fields;
};


}

BOOST_AUTO_TEST_SUITE(TestInherit)

BOOST_AUTO_TEST_CASE(InheritanceV2)
{
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

    auto idxMap=make_index_map(type_c_tuple<type1,type2>::value);
    std::ignore=idxMap;

    WithIndexMap<type1,type2> t3;
    auto& v5=t3.field(type1{});
    static_assert(std::is_same<type1,std::decay_t<decltype(v5)>>::value,"");
    auto& v6=t3.field(type2{});
    static_assert(std::is_same<type2,std::decay_t<decltype(v6)>>::value,"");
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(InheritObjV2)
{
    b1::type v1;
    const auto& fields1=b1::fields;

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
