/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file datauint/detail/unittraits.ipp
  *
  *      Implementations of DataUnit templates
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITSIMPL_H
#define HATNDATAUNITSIMPL_H

#include <hatn/dataunit/unittraits.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/********************** UnitImpl **************************/

//---------------------------------------------------------------
template <typename ...Fields>
UnitImpl<Fields...>::UnitImpl(Unit* self)
    : ::hatn::common::VInterfacesPack<Fields...>(
                    Fields(std::forward<Unit*>(self))...
            )
{
}

//---------------------------------------------------------------
template <typename ...Fields>
template <typename T>
bool UnitImpl<Fields...>::iterate(const T& visitor)
{
    auto predicate=[](bool ok)
    {
        return ok;
    };

    auto handler=[&visitor](auto& field, auto&&)
    {
        return visitor(field);
    };

    return each(predicate,handler);
}

//---------------------------------------------------------------
template <typename ...Fields>
template <typename T>
bool UnitImpl<Fields...>::iterateConst(const T& visitor) const
{
    auto predicate=[](bool ok)
    {
        return ok;
    };

    auto handler=[&visitor](const auto& field, auto&&)
    {
        return visitor(field);
    };

    return each(predicate,handler);
}

//---------------------------------------------------------------

template <typename ...Fields>
const common::FlatMap<int,uintptr_t>& UnitImpl<Fields...>::fieldsMap()
{
    static Unit sampleUnit{};
    static const UnitImpl<Fields...> sample(&sampleUnit);

    auto f = [](common::FlatMap<int,uintptr_t> m, const auto& field) {
        auto m1=std::move(m);
        m1[field.fieldId()]=reinterpret_cast<uintptr_t>(&field)-reinterpret_cast<uintptr_t>(&sample);
        return m1;
    };

    static const common::FlatMap<int,uintptr_t> map=hana::fold(sample.m_interfaces,common::FlatMap<int,uintptr_t>{},f);

    return map;
}

//---------------------------------------------------------------

template <typename ...Fields>
template <typename BufferT>
const common::FlatMap<
        int,
        typename UnitImpl<Fields...>::template FieldParser<BufferT>
    >&
UnitImpl<Fields...>::fieldParsers()
{
    using unitT=UnitImpl<Fields...>;
    using itemT=typename UnitImpl<Fields...>::template FieldParser<BufferT>;

    auto f=[](auto&& state, auto fieldTypeC) {

        using type=typename decltype(fieldTypeC)::type;

        auto index=hana::first(state);
        auto map=hana::second(state);

        auto handler=[index](unitT& unit, BufferT& buf, AllocatorFactory* factory)
        {
            auto& field=unit.template getInterface<decltype(index)::value>();
            return field.deserialize(buf,factory);
        };
        auto item=itemT{
            handler,
            type::fieldWireType(),
            type::fieldName()
        };
        map[type::fieldId()]=item;

        return hana::make_pair(hana::plus(index,hana::int_c<1>),std::move(map));
    };

    static const auto result=hana::fold(
            hana::tuple_t<Fields...>,
            hana::make_pair(
                hana::int_c<0>,
                common::FlatMap<int,itemT>{}
            ),
            f
        );
    static const auto map=hana::second(result);

    return map;
}

/********************** UnitConcat **************************/

//---------------------------------------------------------------
template <typename Conf, typename ...Fields>
UnitConcat<Conf,Fields...>::UnitConcat(
        AllocatorFactory* factory
    ) : Unit(factory),
        UnitImpl<Fields...>(this)
{}

//---------------------------------------------------------------
template <typename Conf, typename ...Fields>
const Field* UnitConcat<Conf,Fields...>::doFieldById(int id) const
{
    return baseType::findField(this,id);
}

//---------------------------------------------------------------
template <typename Conf, typename ...Fields>
Field* UnitConcat<Conf,Fields...>::doFieldById(int id)
{
    return baseType::findField(this,id);
}

//---------------------------------------------------------------
template <typename Conf, typename ...Fields>
size_t UnitConcat<Conf,Fields...>::doFieldCount() const noexcept
{
    return this->count();
}

/********************** EmptyUnit **************************/

/********************** ManagedUnit **************************/

//---------------------------------------------------------------

/********************** EmptyManagedUnit **************************/

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSIMPL_H
