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

    return l->fieldValue(field)==r->fieldValue(field);
}

template <typename MsgT, typename FieldT>
bool fieldLess(const FieldT& field, const MsgT& l, const MsgT& r)
{
    if (!l)
    {
        if (!r )
        {
            return false;
        }
        return true;
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

template <typename LeftT, typename RightT, typename ... ExludeFields>
bool unitsEqual(const LeftT& l, const RightT& r, ExludeFields ...excludeFields)
{
    if (!r)
    {
        if (l)
        {
            return true;
        }
        return false;
    }

    //! @todo Implement recursive in-depth comparing

    auto fieldType=[](auto&& field)
    {
        using type=std::decay_t<decltype(field)>;
        return hana::type_c<type>;
    };

    auto excludeFieldsTs=hana::make_tuple(std::forward<ExludeFields>(excludeFields)...);
    auto excludeTs=hana::transform(excludeFieldsTs,fieldType);

    auto each=[&l,&r,&excludeTs,&fieldType](const auto& field)
    {
        auto fieldTypeC=fieldType(field);
        return hana::eval_if(
            hana::contains(excludeTs,fieldTypeC),
            []()
            {
                return true;
            },
            [&](auto _)
            {
                return _(l)->fieldValue(_(field)) == _(r)->fieldValue(_(field));
            }
        );
    };
    l.iterate(each);
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

    return unitsEqual(&lField.value(),&rField.value());
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
        if (!lEl.isSet())
        {
            if (!rEl.isSet())
            {
                continue;
            }
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

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITCOMPARE_H
