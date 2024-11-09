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
    Error operator()(const T& val) const
    {
        scalarFn(val);
        return OK;
    }

    template <typename T>
    Error operator()(const common::pmr::vector<T>& val) const
    {
        vectorFn(val);
        return OK;
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
    static dataunit::Field* getUnitField(dataunit::Unit* unit, const FieldInfo* fieldInfo, size_t depth=0)
    {
        dataunit::Field* field{nullptr};

        if (!fieldInfo->nested())
        {
            field=unit->fieldById(fieldInfo->id());
        }
        else
        {
            const auto* path=&fieldInfo->path();
            auto* u=unit;
            if (depth==0)
            {
                depth=path->size();
            }
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
        // special cases for operations on repeated fields: replace vector element by index, erase vector element by index, increment vector element by index
        if (updateField.op==Operator::replace_element || updateField.op==Operator::erase_element || updateField.op==Operator::inc_element)
        {
            auto depth=updateField.fieldInfo->path().size();
            Assert(depth>1,"Invalid field path for update operation");
            auto* field=getUnitField(unit,updateField.fieldInfo,depth-1);
            const auto& name=updateField.fieldInfo->path().back();
            size_t idx=std::stoul(name);

            switch (updateField.op)
            {
                case (Operator::replace_element):
                {
                    auto elementSet=[idx,&field](const auto& val)
                    {
                        field->arraySet(idx,val);
                    };
                    auto vectorSet=[](const auto&)
                    {
                    };
                    auto vis=FieldVisitor(std::move(elementSet),std::move(vectorSet));
                    std::ignore=updateField.value.handleValue(vis);
                }
                break;

                case (Operator::erase_element):
                {
                    field->arrayErase(idx);
                }
                break;

                case (Operator::inc_element):
                {
                    std::ignore=updateField.value.handleValue(
                        [idx,&field](const auto& val)
                        {
                            using type=std::decay_t<decltype(val)>;
                            hana::eval_if(
                                std::is_arithmetic<type>{},
                                [&](auto _)
                                {
                                    field->arrayInc(_(idx),_(val));
                                },
                                [&](auto )
                                {
                                    Assert(false,"Increment operation not applicable for this value type");
                                }
                            );
                            return Error{OK};
                        });
                }
                break;

                case (Operator::set):break;
                case (Operator::unset):break;
                case (Operator::inc):break;
                case (Operator::push):break;
                case (Operator::pop):break;
                case (Operator::push_unique):break;
            }

            return;
        }

        // normal operations on the field
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
                auto vis=FieldVisitor(std::move(scalarSet),std::move(vectorSet));
                std::ignore=updateField.value.handleValue(vis);
            }
            break;

            case (Operator::unset):
            {
                field->reset();
            }
            break;

            case (Operator::inc):
            {
                std::ignore=updateField.value.handleValue(
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
                        return Error{OK};
                    });
            }
            break;

            case (Operator::push):
            {
                auto elementAdd=[&field](const auto& val)
                {
                    field->arrayAdd(val);
                };
                auto vectorSet=[](const auto&)
                {
                };
                auto vis=FieldVisitor(std::move(elementAdd),std::move(vectorSet));
                std::ignore=updateField.value.handleValue(vis);
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
                auto elementAdd=[&field](const auto& val)
                {
                    bool unique=true;
                    for (size_t i=0;i<field->arraySize();i++)
                    {
                        if (field->equals(val))
                        {
                            unique=false;
                            break;
                        }
                    }
                    if (unique)
                    {
                        field->arrayAdd(val);
                    }
                };
                auto vectorSet=[](const auto&)
                {
                };
                auto vis=FieldVisitor(std::move(elementAdd),std::move(vectorSet));
                std::ignore=updateField.value.handleValue(vis);
            }
            break;

            case (Operator::replace_element): break;
            case (Operator::erase_element): break;
            case (Operator::inc_element): break;
        }
    }
};
constexpr HandleFieldT HandleField{};

struct ApplyRequestT
{
    void operator() (dataunit::Unit* unit, const Request& request) const
    {
        for (auto&& field: request)
        {
            HandleField(unit,field);
        }
    }
};
constexpr ApplyRequestT ApplyRequest{};

} // namespace update

HATN_DB_NAMESPACE_END

#endif // HATNUPDATEUNIT_IPP
