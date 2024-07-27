/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/detail/updateunit.ipp
  *
  * Contains implementation of unit upading using update request.
  *
  */

/****************************************************************************/

#ifndef HATNUPDATEUNIT_IPP
#define HATNUPDATEUNIT_IPP

#include <hatn/dataunit/unit.h>

#include <hatn/db/update.h>

HATN_DB_NAMESPACE_BEGIN

namespace update
{

struct HandleFieldT
{
    static dataunit::Field* getUnitField(dataunit::Unit* unit, const FieldInfo* fieldInfo)
    {
        dataunit::Field* field{nullptr};

        if (!fieldInfo->nested())
        {
            field=unit->fieldById(fieldInfo->id());
        }
        else
        {
            auto* u=unit;
            auto depth=fieldInfo.path().size();
            auto lastUnit=depth-1;
            for (size_t i=0;i<fieldInfo.path().size();i++)
            {
                //! @todo Implement getting element of repeated field

                field=u->fieldByName(fieldInfo.path().at(i));
                if (field!=nullptr && i!=lastUnit)
                {
                    u=field->subunit();
                }
            }
        }

        Assert(field!=nullptr,"Requested to update unknown field");
        return field;
    }

    void operator() (dataunit::Unit* unit, const Field& updateField) const
    {
        //! @todo Handle element of repeated field

        auto* field=getUnitField(unit,updateField.fieldInfo);

        switch (updateField.op)
        {
            case (Operator::set):
            {
                updateField.value.handleValue(
                    [&field](const auto& val)
                    {
                        field->setV(val);
                    });
            }
            break;

            case (Operator::unset):
            {
                field->reset();
            }
            break;

            case (Operator::inc):
            {
                updateField.value.handleValue(
                    [&field](const auto& val)
                    {
                        using type=std::decay_t<decltype(val)>;
                        hana::eval_if(
                            std::is_arithmetic<type>{},
                            [&](auto _)
                            {
                                field->incV(_(val));
                            },
                            [&](auto )
                            {
                                Assert(false,"Increment operation not applicable for this value type");
                            }
                        );
                    });
            }
            break;

            case (Operator::push):
            {
                updateField.value.handleValue(
                    [&field](const auto& val)
                    {
                        field->arrayAdd(val);
                    });
            }
            break;

            case (Operator::pop):
            {
                auto prevSize=field->arraySize();
                if (prevSize>0)
                {
                    field->arrayResize(prevSize-1);
                }
            }
            break;

            case (Operator::push_unique):
            {
                updateField.value.handleValue(
                    [&field](const auto& val)
                    {
                        using type=std::decay_t<decltype(val)>;
                        type v{};

                        bool unique=true;
                        for (size_t i=0;i<field->arraySize();i++)
                        {
                            field->arrayGet(i,v);
                            if (v==value)
                            {
                                unique=false;
                                break;
                            }
                        }
                        if (unique)
                        {
                            field->arrayAdd(val);
                        }
                    });
            }
            break;
        }
    }
};
constexpr HandleFieldT HandleField{};

struct HandleRequestT
{
    void operator() (dataunit::Unit* unit, const Request& request) const
    {
        for (auto&& field: request)
        {
            HandleField(unit,field);
        }
    }
};
constexpr HandleRequestT HandleRequest{};

} // namespace update

HATN_DB_NAMESPACE_END

#endif // HATNUPDATEUNIT_IPP
