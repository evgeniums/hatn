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

#include <cstring>

#include <hatn/common/stdwrappers.h>

#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/ipp/fieldserialization.ipp>

#include <hatn/db/update.h>
#include <hatn/db/dberror.h>

HATN_DB_NAMESPACE_BEGIN

namespace update
{

template <typename ScalarFnT, typename VectorFnT>
struct FieldVisitorT
{
    template <typename T>
    Error operator()(const T& val) const
    {
        return scalarFn(val);
    }

    template <typename T>
    Error handleVector(const T& val) const
    {
        auto vis=[this](const auto& vec)
        {
            return vectorFn(vec.get());
        };
        return lib::variantVisit(vis,val);
    }

    template <typename T>
    Error operator()(const VectorT<T>& val) const
    {
        return handleVector(val);
    }

    Error operator()(const VectorString& val) const
    {
        return handleVector(val);
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
    //! Parse serialized subunit bytes into an already resolved target unit.
    static Error parseSubunit(dataunit::Unit* target, const SubunitBuf& val)
    {
        if (target==nullptr)
        {
            return dbError(DbError::UPDATE_INVALID_SUBUNIT);
        }
        du::WireBufSolid wbuf(val.data(),val.size(),true);
        if (!du::UnitSer::deserialize(target,wbuf))
        {
            return dbError(DbError::UPDATE_INVALID_SUBUNIT);
        }
        return Error{};
    }

    //! Append a new element to a repeated subunit field and parse val into it.
    static Error appendSubunitBufElement(dataunit::Field* field, const SubunitBuf& val)
    {
        auto idx=field->arraySize();
        field->arrayResize(idx+1);
        return parseSubunit(field->arraySubunit(idx),val);
    }

    //! Serialize a subunit to wire bytes, for byte-wise comparison (see pushUniqueSubunit()).
    static Error serializeSubunitBytes(const dataunit::Unit* u, common::ByteArray& out)
    {
        du::WireBufSolidRef wbuf{out};
        if (!du::UnitSer::serialize(u,wbuf))
        {
            return dbError(DbError::UPDATE_INVALID_SUBUNIT);
        }
        return Error{};
    }

    static bool bytesEqual(const common::ByteArray& buf, const common::ConstDataBuf& other) noexcept
    {
        return buf.size()==other.size() && std::memcmp(buf.data(),other.data(),buf.size())==0;
    }

    //! push_unique for a repeated subunit field: RepeatedGetterSetter<Type>::equals asserts
    //! for unit types (see dataunit/fields/repeated.h), so compare by serialized bytes instead.
    template <typename ValueT>
    static Error pushUniqueSubunitBytes(dataunit::Field* field, const common::ConstDataBuf& candidateBytes, const ValueT& val)
    {
        for (size_t i=0;i<field->arraySize();i++)
        {
            common::ByteArray elemBuf;
            auto ec=serializeSubunitBytes(field->arraySubunit(i),elemBuf);
            HATN_CHECK_EC(ec)
            if (bytesEqual(elemBuf,candidateBytes))
            {
                // already present, nothing to do
                return Error{};
            }
        }

        if constexpr (std::is_same<std::decay_t<ValueT>,SubunitBuf>::value)
        {
            return appendSubunitBufElement(field,val);
        }
        else
        {
            field->arrayAppend(val);
            return Error{};
        }
    }

    static Error pushUniqueSubunit(dataunit::Field* field, const Subunit& val)
    {
        common::ByteArray candidateBuf;
        auto ec=serializeSubunitBytes(val.get(),candidateBuf);
        HATN_CHECK_EC(ec)
        return pushUniqueSubunitBytes(field,common::ConstDataBuf{candidateBuf},val);
    }

    static Error pushUniqueSubunit(dataunit::Field* field, const SubunitBuf& val)
    {
        return pushUniqueSubunitBytes(field,common::ConstDataBuf{val.data(),val.size()},val);
    }

    //! Resolve the field addressed by path, descending through nested subunits and
    //! repeated-subunit elements as needed.
    /**
     * On success (returned Error is OK):
     *  - fieldOut!=nullptr: the field was resolved; the caller applies the requested
     *    operator to it, using path.back().idx for the element operators.
     *  - fieldOut==nullptr: an array index referenced somewhere along the path (other
     *    than the very last path item, which each operator bounds-checks itself) is
     *    out of range for the object being updated; the whole update field is a
     *    silent no-op.
     *
     * On failure (returned Error is not OK): the path does not match the object's
     * schema at all (unknown field id, an array field traversed without db::array(),
     * or a plain field traversed as if it were a subunit).
     */
    static Error resolveField(dataunit::Unit* unit, const FieldPath& path, dataunit::Field*& fieldOut)
    {
        fieldOut=nullptr;

        auto* u=unit;
        dataunit::Field* field=nullptr;

        for (size_t i=0;i<path.size();i++)
        {
            field=u->fieldById(path.at(i).fieldId);
            if (field==nullptr)
            {
                return dbError(DbError::UPDATE_FIELD_NOT_FOUND);
            }

            if (i+1==path.size())
            {
                // last path item: its own idx (if any) is left for the caller, which
                // either targets a specific array element (element operators) or the
                // field as a whole (all other operators).
                break;
            }

            // descend through this field to resolve the next path item. Field::subunit()/
            // arraySubunit() assert (and, in a debug build, abort) rather than returning
            // nullptr when called on a field whose value type isn't a subunit, so that
            // check must happen here, before calling either.
            if (field->valueTypeId()!=dataunit::ValueType::Dataunit)
            {
                return dbError(DbError::UPDATE_FIELD_NOT_FOUND);
            }
            if (field->isArray())
            {
                auto idx=path.at(i).idx;
                if (idx<0)
                {
                    // array field traversed without db::array(): malformed path
                    return dbError(DbError::UPDATE_FIELD_NOT_FOUND);
                }
                if (static_cast<size_t>(idx)>=field->arraySize())
                {
                    // index out of range for this object: silent no-op
                    return Error{};
                }
                u=field->arraySubunit(static_cast<size_t>(idx));
            }
            else
            {
                u=field->subunit();
            }
            if (u==nullptr)
            {
                // field is neither an array of subunits nor a plain subunit
                return dbError(DbError::UPDATE_FIELD_NOT_FOUND);
            }
        }

        fieldOut=field;
        return Error{};
    }

    Error operator() (dataunit::Unit* unit, const Field& updateField) const
    {
        dataunit::Field* field=nullptr;
        auto ec=resolveField(unit,updateField.path,field);
        HATN_CHECK_EC(ec)
        if (field==nullptr)
        {
            // out-of-range array index somewhere along the path: silent no-op
            return Error{};
        }

        // special cases for operations on repeated fields: replace vector element by index, erase vector element by index, increment vector element by index
        if (updateField.op==Operator::replace_element || updateField.op==Operator::erase_element || updateField.op==Operator::inc_element)
        {
            if (!field->isArray())
            {
                return dbError(DbError::UPDATE_FIELD_NOT_FOUND);
            }
            auto idx=static_cast<size_t>(updateField.path.back().idx);

            switch (updateField.op)
            {
                case (Operator::replace_element):
                {
                    auto elementSet=[idx,&field](const auto& val) -> Error
                    {
                        using valueType=std::decay_t<decltype(val)>;
                        if (idx>=field->arraySize())
                        {
                            return Error{};
                        }
                        if constexpr (std::is_same<valueType,SubunitBuf>::value)
                        {
                            return parseSubunit(field->arraySubunit(idx),val);
                        }
                        else
                        {
                            field->arraySet(idx,val);
                            return Error{};
                        }
                    };
                    auto vectorSet=[](const auto&) -> Error
                    {
                        return Error{};
                    };
                    auto vis=FieldVisitor(std::move(elementSet),std::move(vectorSet));
                    return updateField.value.handleValue(vis);
                }

                case (Operator::erase_element):
                {
                    if (idx<field->arraySize())
                    {
                        field->arrayErase(idx);
                    }
                    return Error{};
                }

                case (Operator::inc_element):
                {
                    return updateField.value.handleValue(
                        [idx,&field](const auto& val) -> Error
                        {
                            using type=std::decay_t<decltype(val)>;
                            return hana::eval_if(
                                std::is_arithmetic<type>{},
                                [&](auto _) -> Error
                                {
                                    if (idx<field->arraySize())
                                    {
                                        field->arrayInc(_(idx),_(val));
                                    }
                                    return Error{};
                                },
                                [&](auto ) -> Error
                                {
                                    Assert(false,"Increment operation not applicable for this value type");
                                    return Error{};
                                }
                            );
                        });
                }

                case (Operator::set):break;
                case (Operator::unset):break;
                case (Operator::inc):break;
                case (Operator::push):break;
                case (Operator::pop):break;
                case (Operator::push_unique):break;
            }

            return Error{};
        }

        // normal operations on the field
        switch (updateField.op)
        {
            case (Operator::set):
            {
                auto scalarSet=[&field](const auto& val) -> Error
                {
                    using valueType=std::decay_t<decltype(val)>;
                    if constexpr (std::is_same<valueType,SubunitBuf>::value)
                    {
                        return parseSubunit(field->subunit(),val);
                    }
                    else
                    {
                        field->setV(val);
                        return Error{};
                    }
                };
                auto vectorSet=[&field](const auto& val) -> Error
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
                        else if constexpr (std::is_same<valueT,SubunitBuf>::value)
                        {
                            auto ec1=parseSubunit(field->arraySubunit(i),val[i]);
                            HATN_CHECK_EC(ec1)
                        }
                        else
                        {
                            field->arraySet(i,val[i]);
                        }
                    }
                    return Error{};
                };
                auto vis=FieldVisitor(std::move(scalarSet),std::move(vectorSet));
                return updateField.value.handleValue(vis);
            }

            case (Operator::unset):
            {
                field->reset();
            }
            break;

            case (Operator::inc):
            {
                return updateField.value.handleValue(
                    [&field](const auto& val) -> Error
                    {
                        using type=std::decay_t<decltype(val)>;
                        return hana::eval_if(
                            std::is_arithmetic<type>{},
                            [&](auto _) -> Error
                            {
                                field->incV(_(val));
                                return Error{};
                            },
                            [&](auto ) -> Error
                            {
                                Assert(false,"Increment operation not applicable for this value type");
                                return Error{};
                            }
                        );
                    });
            }

            case (Operator::push):
            {
                auto elementAdd=[&field](const auto& val) -> Error
                {
                    using valueType=std::decay_t<decltype(val)>;
                    if constexpr (std::is_same<valueType,SubunitBuf>::value)
                    {
                        return appendSubunitBufElement(field,val);
                    }
                    else
                    {
                        field->arrayAppend(val);
                        return Error{};
                    }
                };
                auto vectorSet=[](const auto&) -> Error
                {
                    return Error{};
                };
                auto vis=FieldVisitor(std::move(elementAdd),std::move(vectorSet));
                return updateField.value.handleValue(vis);
            }

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
                auto elementAdd=[&field](const auto& val) -> Error
                {
                    using valueType=std::decay_t<decltype(val)>;
                    if constexpr (std::is_same<valueType,Subunit>::value || std::is_same<valueType,SubunitBuf>::value)
                    {
                        return pushUniqueSubunit(field,val);
                    }
                    else
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
                        return Error{};
                    }
                };
                auto vectorSet=[](const auto&) -> Error
                {
                    return Error{};
                };
                auto vis=FieldVisitor(std::move(elementAdd),std::move(vectorSet));
                return updateField.value.handleValue(vis);
            }

            case (Operator::replace_element): break;
            case (Operator::erase_element): break;
            case (Operator::inc_element): break;
        }

        return Error{};
    }
};
constexpr HandleFieldT HandleField{};

struct ApplyRequestT
{
    Error operator() (dataunit::Unit* unit, const Request& request) const
    {
        for (auto&& field: request)
        {
            auto ec=HandleField(unit,field);
            HATN_CHECK_EC(ec)
        }
        return Error{};
    }
};
constexpr ApplyRequestT ApplyRequest{};
constexpr ApplyRequestT applyRequest{};
constexpr ApplyRequestT apply{};

} // namespace update

HATN_DB_NAMESPACE_END

#endif // HATNUPDATEUNIT_IPP
