/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/compare.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITCOMPARE_H
#define HATNDATAUNITCOMPARE_H

#include <hatn/common/meta/foreachif.h>
#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/unit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

template <typename MsgT, typename FieldT>
bool fieldEqual(const FieldT& field, const MsgT& l, const MsgT& r)
{
    if (!r)
    {
        if (!l)
        {
            return true;
        }
        return false;
    }
    if (!l)
    {
        return false;
    }

    return l->fieldValue(field)==r->fieldValue(field);
}

template <typename MsgT, typename FieldT>
bool fieldLess(const FieldT& field, const MsgT& l, const MsgT& r)
{
    if (!l)
    {
        if (!r)
        {
            return false;
        }
        return true;
    }
    if (!r)
    {
        return false;
    }

    return l->fieldValue(field)<r->fieldValue(field);
}

template <typename MsgT, typename ... Fields>
bool fieldsEqual(const MsgT& l, const MsgT& r, Fields&& ...fields)
{
    if (!r)
    {
        if (!l)
        {
            return true;
        }
        return false;
    }
    if (!l)
    {
        return false;
    }

    auto pred=[](bool equal)
    {
        return equal;
    };
    common::foreach_if(
        hana::make_tuple(std::forward<Fields>(fields)...),
        pred,
        [&l,&r](const auto& field)
        {
            return fieldEqual(field,l,r);
        }
    );
}

struct repeatedSubunitFieldsEqualT;

struct unitsEqualT
{
    template <typename LeftT, typename RightT, typename ... ExludeFields>
    bool operator()(const LeftT& l, const RightT& r, ExludeFields&& ...excludeFields) const noexcept;
};
constexpr unitsEqualT unitsEqual{};

struct unitsLessOptionalT
{
    template <typename LeftT, typename RightT, typename ... ExludeFields>
    std::optional<bool> operator()(const LeftT& l, const RightT& r, ExludeFields&& ...excludeFields) const noexcept;
};
constexpr unitsLessOptionalT unitsLessOptional{};

struct unitsLessT
{
    template <typename LeftT, typename RightT, typename ... ExludeFields>
    bool operator ()(const LeftT& l, const RightT& r, ExludeFields&& ...excludeFields) const noexcept
    {
        return unitsLessOptional(l,r,std::forward<ExludeFields>(excludeFields)...).value_or(false);
    }
};
constexpr unitsLessT unitsLess{};

template <typename SubunitFieldT, typename LeftT, typename RightT>
bool subunitsEqual(const SubunitFieldT& subunitField, const LeftT& l, const RightT& r)
{
    if (!r)
    {
        if (!l)
        {
            return true;
        }
        return false;
    }
    if (!l)
    {
        return false;
    }

    const auto& lField=l->field(subunitField);
    const auto& rField=r->field(subunitField);

    if (!lField.isSet())
    {
        if (!rField.isSet())
        {
            return true;
        }
        return false;
    }
    if (!rField.isSet())
    {
        return false;
    }

    return unitsEqual(&lField.value(),&rField.value());
}

struct repeatedSubunitFieldsEqualT
{
    template <typename LeftT, typename RightT, typename ... ExludeFields>
    bool operator() (const LeftT& lField, const RightT& rField, ExludeFields&& ...excludeFields) const noexcept
    {
        if (!lField.isSet())
        {
            if (!rField.isSet())
            {
                return true;
            }
            return false;
        }
        if (!rField.isSet())
        {
            return false;
        }

        if (lField.count()!=rField.count())
        {
            return false;
        }

        for (size_t i=0;i<lField.count();i++)
        {
            const auto& lEl=lField.at(i);
            const auto& rEl=rField.at(i);
            if (!lEl.isSet())
            {
                if (!rEl.isSet())
                {
                    continue;
                }
                return false;
            }
            if (!rEl.isSet())
            {
                return false;
            }

            auto ok=unitsEqual(&lEl.value(),&rEl.value(),std::forward<ExludeFields>(excludeFields)...);
            if (!ok)
            {
                return false;
            }
        }

        return true;
    }
};
constexpr repeatedSubunitFieldsEqualT repeatedSubunitFieldsEqual{};

template <typename FieldT, typename LeftT, typename RightT>
bool repeatedSubunitsEqual(const FieldT& field, const LeftT& l, const RightT& r)
{
    if (!r)
    {
        if (!l)
        {
            return true;
        }
        return false;
    }
    if (!l)
    {
        return false;
    }

    const auto& lField=l->field(field);
    const auto& rField=r->field(field);

    if (!lField.isSet())
    {
        if (!rField.isSet())
        {
            return true;
        }
        return false;
    }
    if (!rField.isSet())
    {
        return false;
    }

    if (lField.count()!=rField.count())
    {
        return false;
    }

    for (size_t i=0;i<lField.count();i++)
    {
        const auto& lEl=lField.at(i);
        const auto& rEl=rField.at(i);
        if (!lEl.isSet())
        {
            if (!rEl.isSet())
            {
                continue;
            }
            return false;
        }
        if (!rEl.isSet())
        {
            return false;
        }

        auto ok=unitsEqual(&lEl.value(),&rEl.value());
        if (!ok)
        {
            return false;
        }
    }

    return true;
}

template <typename FieldT, typename LeftT, typename RightT>
bool repeatedFieldsEqual(const FieldT& field, const LeftT& l, const RightT& r)
{
    if (!r)
    {
        if (!l)
        {
            return true;
        }
        return false;
    }
    if (!l)
    {
        return false;
    }

    const auto& lField=l->field(field);
    const auto& rField=r->field(field);

    if (!lField.isSet())
    {
        if (!rField.isSet())
        {
            return true;
        }
        return false;
    }

    if (lField.count()!=rField.count())
    {
        return false;
    }

    for (size_t i=0;i<lField.count();i++)
    {
        const auto& lEl=lField.at(i);
        const auto& rEl=rField.at(i);
        auto ok=lEl==rEl;
        if (!ok)
        {
            return false;
        }
    }

    return true;
}

template <typename LeftT, typename RightT, typename ... ExludeFields>
bool unitVectorsEqual(const LeftT& l, const RightT& r, ExludeFields&& ...excludeFields)
{
    if (l.size()!=r.size())
    {
        return false;
    }

    for (size_t i=0;i<l.size();i++)
    {
        const auto& lEl=l.at(i);
        const auto& rEl=r.at(i);
        auto ok=unitsEqual(lEl,rEl,std::forward<ExludeFields>(excludeFields)...);
        if (!ok)
        {
            return false;
        }
    }

    return true;
}

template <typename LeftT, typename RightT, typename ... ExludeFields>
bool unitsEqualT::operator()(const LeftT& l, const RightT& r, ExludeFields&& ...excludeFields) const noexcept
{
    using type=std::decay_t<decltype(*l)>;

    if constexpr (std::is_same<type,Unit>::value)
    {
        // abstract units are skipped
        return true;
    }
    else
    {
        if (!r)
        {
            if (!l)
            {
                return true;
            }
            return false;
        }
        if (!l)
        {
            return false;
        }

        auto excludeFieldsC=hana::transform(
            hana::make_tuple(std::forward<ExludeFields>(excludeFields)...),
            [](auto field)
            {
                return hana::type_c<std::decay_t<decltype(field)>>;
            }
        );

        constexpr static auto hasGet = boost::hana::is_valid([](auto&& obj, auto&& index)
                                                             -> decltype(obj->get(index)) { });

        auto each=[&](auto index)
        {
            return hana::eval_if(
                hana::not_(hasGet(l,index)),
                [&](auto)
                {
                    return true;
                },
                [&](auto _)
                {
                    using fieldType=std::decay_t<decltype(_(l)->get(_(index)))>;

                    return hana::eval_if(
                        typename fieldType::isUnitType{},
                        [&](auto)
                        {
                            return hana::eval_if(
                                typename fieldType::isRepeatedType{},
                                [&](auto _)
                                {
                                    return hana::unpack(
                                        _(excludeFieldsC),
                                        hana::partial(
                                            repeatedSubunitFieldsEqual,
                                            _(l)->get(_(index)),
                                            _(r)->get(_(index))
                                            )
                                        );
                                },
                                [&](auto _)
                                {
                                    return hana::unpack(
                                        _(excludeFieldsC),
                                        hana::partial(
                                            unitsEqual,
                                            _(l)->get(_(index)).sharedValue(),
                                            _(r)->get(_(index)).sharedValue()
                                            )
                                        );
                                    }
                                );
                        },
                        [&](auto _)
                        {
                            return _(l)->get(_(index)).value() == r->get(_(index)).value();
                        }
                        );
                }
                );
        };

        return hana::fold(
            l->fieldsMap,
            true,
            [each,&excludeFieldsC](bool state, auto key)
            {
                if (!state)
                {
                    return false;
                }

                auto fieldC=hana::first(key);
                return hana::eval_if(
                    hana::or_(
                        hana::contains(excludeFieldsC,fieldC)
                    ),
                    [&](auto _)
                    {
                        return _(state);
                    },
                    [&](auto)
                    {
                        auto index=hana::second(key);
                        return each(index);
                    }
                );
            }
        );
    }
}

template <typename LeftT, typename RightT, typename ... ExludeFields>
std::optional<bool> unitsLessOptionalT::operator()(const LeftT& l, const RightT& r, ExludeFields&& ...excludeFields) const noexcept
{
    using type=std::decay_t<decltype(*l)>;

    if constexpr (std::is_same<type,Unit>::value)
    {
        // abstract units are skipped
        return std::nullopt;
    }
    else
    {
        if (!l)
        {
            if (!r)
            {
                return false;
            }
            return true;
        }
        if (!r)
        {
            return false;
        }

        auto excludeFieldsC=hana::transform(
            hana::make_tuple(std::forward<ExludeFields>(excludeFields)...),
            [](auto field)
            {
                return hana::type_c<std::decay_t<decltype(field)>>;
            }
        );

        auto hasGet = boost::hana::is_valid([](auto&& obj, auto&& index)
                                            -> decltype(obj->get(index)) { });
        auto each=[&](auto index) -> lib::optional<bool>
        {
            return hana::eval_if(
                boost::hana::or_(
                    hana::not_(hasGet(l,index))
                    ),
                [&](auto)
                {
                    return lib::optional<bool>{};
                },
                [&](auto _) -> lib::optional<bool>
                {
                    using fieldType=std::decay_t<decltype(_(l)->get(_(index)))>;

                    return hana::eval_if(
                        typename fieldType::isUnitType{},
                        [&](auto _)
                        {
                            return hana::unpack(
                                _(excludeFieldsC),
                                hana::partial(
                                    unitsLessOptional,
                                    _(l)->get(_(index)).sharedValue(),
                                    _(r)->get(_(index)).sharedValue()
                                )
                            );
                        },
                        [&](auto _) -> lib::optional<bool>
                        {
                            if (_(l)->get(_(index)).value() < r->get(_(index)).value())
                            {
                                return true;
                            }
                            if (_(l)->get(_(index)).value() > r->get(_(index)).value())
                            {
                                return false;
                            }
                            return lib::optional<bool>{};
                        }
                    );
                }
            );
        };

        lib::optional<bool> result=hana::fold(
            l->fieldsMap,
            lib::optional<bool>{},
            [each,&excludeFieldsC](lib::optional<bool> state, auto key)
            {
                if (state)
                {
                    return state;
                }

                auto fieldC=hana::first(key);
                return hana::eval_if(
                    hana::contains(excludeFieldsC,fieldC),
                    [&](auto _)
                    {
                        return _(state);
                    },
                    [&](auto )
                    {
                        auto index=hana::second(key);
                        return each(index);
                    }
                );
            }
        );

        return result;
    }
}

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITCOMPARE_H
