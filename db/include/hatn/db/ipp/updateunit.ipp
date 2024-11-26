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

#include <hatn/common/stdwrappers.h>

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
    void handleVector(const T& val) const
    {
        auto vis=[this](const auto& vec)
        {
            vectorFn(vec.get());
        };
        lib::variantVisit(vis,val);
    }

    template <typename T>
    Error operator()(const VectorT<T>& val) const
    {
        handleVector(val);
        return OK;
    }

    Error operator()(const VectorString& val) const
    {
        handleVector(val);
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
    static dataunit::Field* getUnitField(dataunit::Unit* unit, const Field& updateField, bool maxDepth=true)
    {
        auto* field=unit->fieldById(updateField.path.at(0).fieldId);
        Assert(field!=nullptr,"Field not found in the object");

        size_t count=maxDepth? updateField.path.size() : (updateField.path.size()-1);

        auto* u=unit;
        for (size_t i=1;i<count;i++)
        {
            if (field->isArray())
            {
                size_t id=static_cast<size_t>(updateField.path.at(i).id);
                u=field->arraySubunit(id);
                i++;
            }
            else
            {
                u=field->subunit();
            }
            Assert(u!=nullptr,"Field must be of subunit type");
            field=u->fieldById(updateField.path.at(i).fieldId);
        }

        Assert(field!=nullptr,"Unknown field for update");
        return field;
    }

    void operator() (dataunit::Unit* unit, const Field& updateField) const
    {
        // special cases for operations on repeated fields: replace vector element by index, erase vector element by index, increment vector element by index
        if (updateField.op==Operator::replace_element || updateField.op==Operator::erase_element || updateField.op==Operator::inc_element)
        {
            auto* field=getUnitField(unit,updateField,false);
            auto id=static_cast<size_t>(updateField.path.back().id);

            switch (updateField.op)
            {
                case (Operator::replace_element):
                {
                    auto elementSet=[id,&field](const auto& val)
                    {
                        if (id<field->arraySize())
                        {
                            field->arraySet(id,val);
                        }
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
                    if (id<field->arraySize())
                    {
                        field->arrayErase(id);
                    }
                }
                break;

                case (Operator::inc_element):
                {
                    std::ignore=updateField.value.handleValue(
                        [id,&field](const auto& val)
                        {
                            using type=std::decay_t<decltype(val)>;
                            hana::eval_if(
                                std::is_arithmetic<type>{},
                                [&](auto _)
                                {
                                    if (id<field->arraySize())
                                    {
                                        field->arrayInc(_(id),_(val));
                                    }
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
        auto* field=getUnitField(unit,updateField);
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
                        // to avoid some compiler warnings on bool type conversions
                        using valueT=typename std::decay_t<decltype(val)>::value_type;
                        if constexpr (std::is_same<valueT,bool>::value)
                        {
                            field->arraySet(i,static_cast<bool>(val[i]));
                        }
                        else
                        {
                            field->arraySet(i,val[i]);
                        }
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
                    field->arrayAppend(val);
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
                        if (field->arrayEquals(i,val))
                        {
                            unique=false;
                            break;
                        }
                    }
                    if (unique)
                    {
                        field->arrayAppend(val);
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
constexpr ApplyRequestT applyRequest{};
constexpr ApplyRequestT apply{};

} // namespace update

HATN_DB_NAMESPACE_END

#endif // HATNUPDATEUNIT_IPP
