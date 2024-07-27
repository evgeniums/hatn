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

template <typename ScalarFnT, typename VectorFnT>
struct FieldVisitorT
{
    template <typename T>
    void operator()(const T& val) const
    {
        scalarFn(val);
    }

    template <typename T>
    void operator()(const common::pmr::vector<T>& val) const
    {
        vectorFn(val);
    }

    template <typename T1, typename T2>
    FieldVisitorT(T1&& scalar, T2&& vector):
        scalarFn(std::forward<T1>(scalar)),
        vectorFn(std::forward<T2>(vector))
    {}

    ScalarFnT scalarFn;
    VectorFnT vectorFn;
};

template <typename T1, typename T2>
auto FieldVisitor(T1&& scalar, T2&& vector)
{
    return FieldVisitorT<std::decay_t<T1>,std::decay_t<T2>>{std::forward<T1>(scalar),std::forward<T2>(vector)};
}

struct HandleFieldT
{
    static dataunit::Field* getUnitField(dataunit::Unit* unit, const FieldInfo* fieldInfo, const std::vector<std::string>* path=nullptr)
    {
        dataunit::Field* field{nullptr};

        if (!fieldInfo->nested())
        {
            field=unit->fieldById(fieldInfo->id());
        }
        else
        {
            if (path==nullptr)
            {
                path=&fieldInfo->path();
            }

            auto* u=unit;
            auto depth=path->size();
            for (size_t i=0;i<depth;i++)
            {
                if (field!=nullptr)
                {
                    if (field->isArray())
                    {
                        const auto& name=path->at(i);
                        size_t idx=std::stoul(name);
                        u=field->arraySubunit(idx);
                        i++;
                        if (i==depth)
                        {
                            break;
                        }
                    }
                    else
                    {
                        u=field->subunit();
                    }
                }
                const auto& name=path->at(i);
                field=u->fieldByName(name);
            }
        }

        Assert(field!=nullptr,"Requested to update unknown field");
        return field;
    }

    void operator() (dataunit::Unit* unit, const Field& updateField) const
    {
        //! @todo Add operations for repeated fields: replace vector element by index, erase vector element by index.

        auto* field=getUnitField(unit,updateField.fieldInfo);

        switch (updateField.op)
        {
            case (Operator::set):
            {
                auto scalarSet=[&field](const auto& val)
                {
                    field->setV(val);
                };

                auto vectorSet=[&field](const auto& val)
                {
                    field->arrayResize(val.size());
                    for (size_t i=0;i<val.size();i++)
                    {
                        field->arraySet(i,val[i]);
                    }
                };

                updateField.value.handleValue(
                    FieldVisitor(std::move(scalarSet),std::move(vectorSet));
                );
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
