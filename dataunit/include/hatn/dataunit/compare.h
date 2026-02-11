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

template <typename LeftT, typename RightT>
bool repeatedSubunitFieldsEqual(const LeftT& lField, const RightT& rField);

template <typename LeftT, typename RightT, typename ... ExludeFields>
bool unitsEqual(const LeftT& l, const RightT& r, ExludeFields ...excludeFields)
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

    auto excludeIndexes=hana::transform(
        hana::make_tuple(std::forward<ExludeFields>(excludeFields)...),
        [](auto field)
        {
            return hana::size_c<std::decay_t<decltype(field)>::type::ID>;
        }
    );

    auto hasGet = boost::hana::is_valid([](auto&& obj, auto&& index)
                                        -> decltype(obj.get(index)) { });
    auto each=[&](auto index)
    {
        return hana::eval_if(
            boost::hana::or_(
                hana::contains(excludeIndexes,index),
                hana::not_(hasGet(l,index))
                ),
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
                                return repeatedSubunitFieldsEqual(
                                    _(l)->get(_(index)),
                                    _(r)->get(_(index))
                                );
                            },
                            [&](auto _)
                            {
                                return unitsEqual(
                                    _(l)->get(_(index)).sharedValue(),
                                    _(r)->get(_(index)).sharedValue()
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
        [each](bool state, auto key)
        {
            auto index=hana::second(key);
            return state && each(index);
        }
    );
}

template <typename LeftT, typename RightT, typename ... ExludeFields>
std::optional<bool> unitsLessOptional(const LeftT& l, const RightT& r, ExludeFields ...excludeFields)
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

    auto excludeIndexes=hana::transform(
        hana::make_tuple(std::forward<ExludeFields>(excludeFields)...),
        [](auto field)
        {
            return hana::size_c<std::decay_t<decltype(field)>::type::ID>;
        }
    );

    auto hasGet = boost::hana::is_valid([](auto&& obj, auto&& index)
                                      -> decltype(obj.get(index)) { });
    auto each=[&](auto index)
    {
        return hana::eval_if(
            boost::hana::or_(
                hana::contains(excludeIndexes,index),
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
                        return unitsLessOptional(
                                         _(l)->get(_(index)).sharedValue(),
                                         _(r)->get(_(index)).sharedValue()
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

    auto result=hana::fold(
        l->fieldsMap,
        lib::optional<bool>{},
        [each](lib::optional<bool> state, auto key)
        {
            auto index=hana::second(key);
            if (state)
            {
                return state;
            }
            return each(index);
        }
    );

    return result;
}

template <typename LeftT, typename RightT, typename ... ExludeFields>
bool unitsLess(const LeftT& l, const RightT& r, ExludeFields ...excludeFields)
{
    return unitsLessOptional(l,r,std::forward<ExludeFields>(excludeFields)...).value_or(false);
}

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

template <typename LeftT, typename RightT>
bool repeatedSubunitFieldsEqual(const LeftT& lField, const RightT& rField)
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

        auto ok=unitsEqual(&lEl.value(),&rEl.value());
        if (!ok)
        {
            return false;
        }
    }

    return true;
}

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

template <typename LeftT, typename RightT>
bool unitVectorsEqual(const LeftT& l, const RightT& r)
{
    if (l.size()!=r.size())
    {
        return false;
    }

    for (size_t i=0;i<l.size();i++)
    {
        const auto& lEl=l.at(i);
        const auto& rEl=r.at(i);
        auto ok=unitsEqual(lEl,rEl);
        if (!ok)
        {
            return false;
        }
    }

    return true;
}

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITCOMPARE_H
