/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/detail/updateserialization.ipp
  *
  */

/****************************************************************************/

#ifndef HATNUPDATESERIALIZATION_IPP
#define HATNUPDATESERIALIZATION_IPP

#include <hatn/common/stdwrappers.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/dataunit/fieldserialization.h>
#include <hatn/dataunit/fields/date.h>
#include <hatn/dataunit/fields/datetime.h>
#include <hatn/dataunit/fields/daterange.h>
#include <hatn/dataunit/fields/time.h>

#include <hatn/db/updateserialization.h>
#include <hatn/db/ipp/updateunit.ipp>

HATN_DB_NAMESPACE_BEGIN

namespace update
{

namespace serialization
{

template <typename BufT>
struct SerializeScalar
{
    SerializeScalar(BufT& buf):buf(buf)
    {}

    BufT& buf;

    template <typename T>
    Error variableSer(T val) const
    {
        if (!du::VariableSer<T>::serialize(val,buf))
        {
            return du::unitError(du::UnitError::SERIALIZE_ERROR);
        }
        return OK;
    }

    template <typename T>
    Error fixedSer(T val) const
    {
        if (!du::FixedSer<T>::serialize(val,buf))
        {
            return du::unitError(du::UnitError::SERIALIZE_ERROR);
        }
        return OK;
    }

    Error operator () (bool val) const
    {
        return variableSer(static_cast<uint32_t>(val));
    }
    Error operator () (int8_t val) const
    {
        return variableSer(val);
    }
    Error operator () (int16_t val) const
    {
        return variableSer(val);
    }
    Error operator () (int32_t val) const
    {
        return variableSer(val);
    }
    Error operator () (int64_t val) const
    {
        return variableSer(val);
    }
    Error operator () (uint8_t val) const
    {
        return variableSer(val);
    }
    Error operator () (uint16_t val) const
    {
        return variableSer(val);
    }
    Error operator () (uint32_t val) const
    {
        return variableSer(val);
    }
    Error operator () (uint64_t val) const
    {
        return variableSer(val);
    }

    Error operator () (float val) const
    {
        return fixedSer(val);
    }
    Error operator () (double val) const
    {
        return fixedSer(val);
    }

    Error operator () (const String& val) const
    {
        if (!du::BytesSer<String,common::ByteArrayShared>::serialize(buf,&val,common::ByteArrayShared{},false))
        {
            return du::unitError(du::UnitError::SERIALIZE_ERROR);
        }
        return OK;
    }

    Error operator () (const Subunit& val) const
    {
        if (!du::UnitSer::serialize(val.get(),buf))
        {
            return du::unitError(du::UnitError::SERIALIZE_ERROR);
        }
        return OK;
    }

    Error operator () (const SubunitBuf& val) const
    {
        // val already holds the exact bytes that du::UnitSer::serialize would
        // produce (this is how a deserialized update request carries a
        // Subunit/VectorSubunit operand whose concrete unit type was unknown at
        // deserialization time, see update::SubunitBuf), so copy them through as-is.
        buf.mainContainer()->append(val.data(),val.size());
        buf.incSize(static_cast<int>(val.size()));
        return OK;
    }

    Error operator () (const ObjectId& val) const
    {
        if (!du::OidTraits::serialize(val,buf))
        {
            return du::unitError(du::UnitError::SERIALIZE_ERROR);
        }
        return OK;
    }

    Error operator () (const common::Date& val) const
    {
        if (!du::DateTraits::serialize(val,buf))
        {
            return du::unitError(du::UnitError::SERIALIZE_ERROR);
        }
        return OK;
    }

    Error operator () (const common::DateTime& val) const
    {
        if (!du::DateTime::serialize(val,buf))
        {
            return du::unitError(du::UnitError::SERIALIZE_ERROR);
        }
        return OK;
    }

    Error operator () (const common::Time& val) const
    {
        if (!du::TimeTraits::serialize(val,buf))
        {
            return du::unitError(du::UnitError::SERIALIZE_ERROR);
        }
        return OK;
    }

    Error operator () (const common::DateRange& val) const
    {
        if (!du::DateRangeTraits::serialize(val,buf))
        {
            return du::unitError(du::UnitError::SERIALIZE_ERROR);
        }
        return OK;
    }
};

struct serializeFieldT
{
    Error operator() (const Field& updateField, message::type& msg) const
    {
        auto valueType=updateField.value.typeId();

        // SubunitBuf/VectorSubunitBuf are internal carrier types used only in-process
        // when a request was deserialized from the wire without knowing the concrete
        // unit type (see update::SubunitBuf); on the wire they are indistinguishable
        // from a genuine Subunit/VectorSubunit operand, so re-serializing a
        // deserialized request reproduces the original wire tag.
        if (valueType==ValueType::SubunitBuf)
        {
            valueType=ValueType::Subunit;
        }
        else if (valueType==ValueType::VectorSubunitBuf)
        {
            valueType=ValueType::VectorSubunit;
        }

        auto& field = msg.field(message::the_fields).createAndAppendValue();
        field.setFieldValue(a_field::op,updateField.op);
        field.setFieldValue(a_field::value_type,valueType);
        auto& path=field.mutableField(a_field::path);
        for (auto&& it: updateField.path)
        {
            auto& item=path.createAndAppendValue();
            item.setFieldValue(field_id::id,it.fieldId);
            // idx==-1 marks a path element as not indexed (see db::makePathT); cast
            // explicitly so the sentinel round-trips as the fixed bit pattern
            // 0xFFFFFFFF through the wire's uint32 idx field.
            item.setFieldValue(field_id::idx,static_cast<uint32_t>(it.idx));
            item.setFieldValue(field_id::field_name,it.name);
        }

        if (updateField.op==Operator::unset
            ||
            updateField.op==Operator::pop
            ||
            updateField.op==Operator::erase_element
            )
        {
            return Error{};
        }

        auto scalarFn=[&field](const auto& val) -> Error
        {
            auto& scalar=field.mutableValue()->field(a_field::scalar);
            common::ByteArray* buf=scalar.buf();
            du::WireBufSolidRef wbuf{*buf};
            return SerializeScalar{wbuf}(val);
        };
        auto vectorFn=[&field](const auto& val) -> Error
        {
            auto& vector=field.mutableValue()->field(a_field::vector);
            for (const auto& it: val)
            {
                auto& scalar=vector.createAndAppendValue();
                common::ByteArray* buf=scalar.buf();
                du::WireBufSolidRef wbuf{*buf};
                auto ec=SerializeScalar{wbuf}(it);
                HATN_CHECK_EC(ec)
            }
            return Error{};
        };

        auto vis=FieldVisitor(std::move(scalarFn),std::move(vectorFn));
        return updateField.value.handleValue(vis);
    }
};
constexpr serializeFieldT serializeField{};

struct serializeT
{
    Error operator() (const Request& request, message::type& msg) const
    {
        msg.clear();
        for (const auto& field: request)
        {
            auto ec=serializeField(field,msg);
            HATN_CHECK_EC(ec)
        }
        return OK;
    }
};

template <typename T>
using VectorHolderT=common::pmr::vector<T>;
using VectorsHolder=lib::variant<
    VectorHolderT<int8_t>,
    VectorHolderT<int16_t>,
    VectorHolderT<int32_t>,
    VectorHolderT<int64_t>,
    VectorHolderT<uint8_t>,
    VectorHolderT<uint16_t>,
    VectorHolderT<uint32_t>,
    VectorHolderT<uint64_t>,
    VectorHolderT<float>,
    VectorHolderT<double>,
    VectorHolderT<String>,
    VectorHolderT<common::DateTime>,
    VectorHolderT<common::Date>,
    VectorHolderT<common::Time>,
    VectorHolderT<common::DateRange>,
    VectorHolderT<ObjectId>,
    VectorHolderT<SubunitBuf>
>;

template <typename T, typename T1=T>
struct varDeserT
{
    template <typename HandlerT>
    Error operator() (const common::ByteArray& buf, HandlerT handler) const
    {
        T val{0};
        du::WireBufSolidConstRef wbuf(buf);
        if (!du::VariableSer<T>::deserialize(val,wbuf))
        {
            return du::unitError(du::UnitError::PARSE_ERROR);
        }
        handler(static_cast<T1>(val));
        return OK;
    }
};
template <typename T, typename T1=T>
constexpr varDeserT<T,T1> varDeser{};

template <typename T, typename T1=T>
struct fixedDeserT
{
    template <typename HandlerT>
    Error operator() (const common::ByteArray& buf, HandlerT handler) const
    {
        T val{0};
        du::WireBufSolidConstRef wbuf(buf);
        if (!du::FixedSer<T>::deserialize(val,wbuf))
        {
            return du::unitError(du::UnitError::PARSE_ERROR);
        }
        handler(static_cast<T1>(val));
        return OK;
    }
};
template <typename T, typename T1=T>
constexpr fixedDeserT<T,T1> fixedDeser{};

struct deserializeT
{
    //! Deserialize an update request from a wire message.
    /**
     * @param msg Wire message to deserialize from.
     * @param request Resulting request.
     * @param vectorsHolder Storage for the vector operands referenced by request (must
     *        outlive request, see update::Field::value / update::ValueVariant).
     * @param factory Allocator factory used for vectorsHolder's elements.
     *
     * Note: a Subunit/VectorSubunit operand is carried through as a raw-bytes
     * SubunitBuf/Vector<SubunitBuf> view into msg's own storage (the concrete unit
     * type is not known at this point), so msg must also outlive request whenever it
     * contains such an operand.
     */
    Error operator() (const message::type& msg, Request& request,
                                             common::pmr::vector<VectorsHolder>& vectorsHolder,
                                             const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) const
    {
        request.clear();
        vectorsHolder.clear();

        const auto& fields=msg.field(message::the_fields).value();
        common::ByteArray tmpInlineString;
        common::ByteArrayShared fooBuf;
        const char* nullbuf=nullptr;
        tmpInlineString.loadInline(nullbuf,0);
        request.reserve(fields.size());
        vectorsHolder.reserve(fields.size());

        for (size_t i=0;i<fields.size();i++)
        {
            const auto& field=fields.at(i);

            // construct path
            const auto& fieldPath=field.field(a_field::path).value();
            FieldPath path;
            for (auto&& it1: fieldPath)
            {
                path.emplace_back(it1.fieldValue(field_id::id),
                                it1.field(field_id::field_name).c_str(),
                                static_cast<int>(it1.fieldValue(field_id::idx))
                               );
            }

            // lambda used to parse scalar value or element of vector
            auto parseValue=[&tmpInlineString,&fooBuf,factory](const common::ByteArray& buf, ValueType type, auto handler)
            {
                switch (type)
                {
                    case(ValueType::Int8_t): HATN_FALLTHROUGH
                        case(ValueType::VectorInt8_t):
                        case(ValueType::Bool):
                        case(ValueType::VectorBool):
                    {
                        auto ec=varDeser<int8_t,bool>(buf,std::move(handler));
                        HATN_CHECK_EC(ec);
                    }
                    break;
                    case(ValueType::Uint8_t): HATN_FALLTHROUGH
                        case(ValueType::VectorUint8_t):
                    {
                        HATN_CHECK_RETURN(varDeser<uint8_t>(buf,std::move(handler)));
                    }
                    break;
                    case(ValueType::Int16_t): HATN_FALLTHROUGH
                        case(ValueType::VectorInt16_t):
                    {
                        HATN_CHECK_RETURN(varDeser<int16_t>(buf,std::move(handler)));
                    }
                    break;
                    case(ValueType::Uint16_t): HATN_FALLTHROUGH
                        case(ValueType::VectorUint16_t):
                    {
                        HATN_CHECK_RETURN(varDeser<uint16_t>(buf,std::move(handler)));
                    }
                    break;
                    case(ValueType::Int32_t): HATN_FALLTHROUGH
                        case(ValueType::VectorInt32_t):
                    {
                        HATN_CHECK_RETURN(varDeser<int32_t>(buf,std::move(handler)));
                    }
                    break;
                    case(ValueType::Uint32_t): HATN_FALLTHROUGH
                        case(ValueType::VectorUint32_t):
                    {
                        HATN_CHECK_RETURN(varDeser<uint32_t>(buf,std::move(handler)));
                    }
                    break;
                    case(ValueType::Int64_t): HATN_FALLTHROUGH
                        case(ValueType::VectorInt64_t):
                    {
                        HATN_CHECK_RETURN(varDeser<int64_t>(buf,std::move(handler)));
                    }
                    break;
                    case(ValueType::Uint64_t): HATN_FALLTHROUGH
                        case(ValueType::VectorUint64_t):
                    {
                        HATN_CHECK_RETURN(varDeser<uint64_t>(buf,std::move(handler)));
                    }
                    break;

                    case(ValueType::Float): HATN_FALLTHROUGH
                        case(ValueType::VectorFloat):
                    {
                        HATN_CHECK_RETURN(fixedDeser<float>(buf,std::move(handler)));
                    }
                    break;
                    case(ValueType::Double): HATN_FALLTHROUGH
                        case(ValueType::VectorDouble):
                    {
                        HATN_CHECK_RETURN(fixedDeser<double>(buf,std::move(handler)));
                    }
                    break;

                    case(ValueType::String): HATN_FALLTHROUGH
                        case(ValueType::VectorString):
                    {
                        du::WireBufSolidConstRef wbuf(buf);
                        wbuf.setUseInlineBuffers(true);
                        if (!du::BytesSer<common::ByteArray,common::ByteArrayShared>::deserialize(wbuf,&tmpInlineString,fooBuf,factory,0,false))
                        {
                            return du::unitError(du::UnitError::PARSE_ERROR);
                        }
                        handler(tmpInlineString.stringView());
                        return Error{};
                    }
                    break;

                    case(ValueType::Subunit): HATN_FALLTHROUGH
                        case(ValueType::VectorSubunit):
                        // SubunitBuf/VectorSubunitBuf never actually appear on the wire
                        // (serializeFieldT remaps them to Subunit/VectorSubunit, see
                        // update::SubunitBuf), but handle them the same way regardless
                        // in case a message is ever re-deserialized from an in-process
                        // Request that still carries the internal tag.
                        case(ValueType::SubunitBuf): HATN_FALLTHROUGH
                        case(ValueType::VectorSubunitBuf):
                    {
                        // the concrete unit type is not known at this point, so hand
                        // the raw already-serialized bytes through as SubunitBuf;
                        // du::UnitSer::deserialize (used by update::HandleFieldT)
                        // parses them once the target field's type is known. The
                        // referenced bytes are owned by msg, which must therefore
                        // outlive the resulting request (see deserializeT's docs).
                        handler(SubunitBuf{buf.data(),buf.size()});
                        return Error{};
                    }
                    break;

                    case(ValueType::ObjectId): HATN_FALLTHROUGH
                        case(ValueType::VectorObjectId):
                    {
                        ObjectId val;
                        du::WireBufSolidConstRef wbuf(buf);
                        if (!du::OidTraits::deserialize(val,wbuf))
                        {
                            return du::unitError(du::UnitError::SERIALIZE_ERROR);
                        }
                        handler(val);
                        return Error{};
                    }
                    break;

                    case(ValueType::Date): HATN_FALLTHROUGH
                        case(ValueType::VectorDate):
                    {
                        common::Date val;
                        du::WireBufSolidConstRef wbuf(buf);
                        if (!du::DateTraits::deserialize(val,wbuf))
                        {
                            return du::unitError(du::UnitError::SERIALIZE_ERROR);
                        }
                        handler(val);
                        return Error{};
                    }
                    break;

                    case(ValueType::DateRange): HATN_FALLTHROUGH
                        case(ValueType::VectorDateRange):
                    {
                        common::DateRange val;
                        du::WireBufSolidConstRef wbuf(buf);
                        if (!du::DateRangeTraits::deserialize(val,wbuf))
                        {
                            return du::unitError(du::UnitError::SERIALIZE_ERROR);
                        }
                        handler(val);
                        return Error{};
                    }
                    break;

                    case(ValueType::Time): HATN_FALLTHROUGH
                        case(ValueType::VectorTime):
                    {
                        common::Time val;
                        du::WireBufSolidConstRef wbuf(buf);
                        if (!du::TimeTraits::deserialize(val,wbuf))
                        {
                            return du::unitError(du::UnitError::SERIALIZE_ERROR);
                        }
                        handler(val);
                        return Error{};
                    }
                    break;

                    case(ValueType::DateTime): HATN_FALLTHROUGH
                        case(ValueType::VectorDateTime):
                    {
                        common::DateTime val;
                        du::WireBufSolidConstRef wbuf(buf);
                        if (!du::DateTime::deserialize(val,wbuf))
                        {
                            return du::unitError(du::UnitError::SERIALIZE_ERROR);
                        }
                        handler(val);
                        return Error{};
                    }
                    break;
                }

                return Error{};
            };

            auto op=field.field(a_field::op).value();
            if (op==Operator::unset
                ||
                op==Operator::erase_element
                ||
                op==Operator::pop
                )
            {
                request.emplace_back(std::move(path),field.field(a_field::op).value());
            }
            else if (field.field(a_field::scalar).isSet())
            {
                // parse scalar field
                auto handler=[&request,&field,&path](const auto& val)
                {
                    request.emplace_back(std::move(path),field.field(a_field::op).value(),val);
                };
                auto ec=parseValue(field.field(a_field::scalar).byteArray(),field.fieldValue(a_field::value_type),std::move(handler));
                HATN_CHECK_EC(ec)
            }
            else if (field.field(a_field::vector).isSet())
            {
                // parse vector field

                // extract vector from message
                const auto& vector=field.field(a_field::vector).value();

                // handle vector of values before adding it to request's field
                auto vectorHandler=[&vectorsHolder,&request,&path,&field,&vector,&parseValue](auto vec)
                {
                    // parse each element of the vector
                    using vecValType=typename std::decay_t<decltype(vec)>::value_type;
                    auto itemHandler=[&vec](const auto& val)
                    {
                        using valType=std::decay_t<decltype(val)>;
                        hana::eval_if(
                            std::is_convertible<valType,vecValType>{},
                            [&](auto _)
                            {
                                _(vec).push_back(static_cast<vecValType>(_(val)));
                            },
                            [](){}
                        );

                    };

                    // parse each element of the vector
                    for (const auto& it: vector)
                    {
                        auto ec=parseValue(it.byteArray(),field.fieldValue(a_field::value_type),itemHandler);
                        HATN_CHECK_EC(ec)
                    }

                    // append vector holder to holders
                    const auto& varVec=vectorsHolder.emplace_back(std::move(vec));

                    // append vector field to request
                    lib::variantVisit(
                        [&request,&field,&path](const auto& v)
                        {
                            request.emplace_back(std::move(path),field.field(a_field::op).value(),std::cref(v));
                        },
                        varVec
                    );

                    // done vectorHandler
                    return Error{};
                };

                // create vector holder depending on value type and then invoke vectorHandler
                switch (field.fieldValue(a_field::value_type))
                {
                    case(ValueType::VectorInt8_t):
                    case(ValueType::VectorBool):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createDataVector<int8_t>()))
                    }
                    break;
                    case(ValueType::VectorUint8_t):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createDataVector<uint8_t>()))
                    }
                    break;
                    case(ValueType::VectorInt16_t):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createDataVector<int16_t>()))
                    }
                    break;
                    case(ValueType::VectorUint16_t):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createDataVector<int16_t>()))
                    }
                    break;
                    case(ValueType::VectorInt32_t):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createDataVector<int32_t>()))
                    }
                    break;
                    case(ValueType::VectorUint32_t):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createDataVector<uint32_t>()));
                    }
                    break;
                    case(ValueType::VectorInt64_t):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createDataVector<int64_t>()));
                    }
                    break;
                    case(ValueType::VectorUint64_t):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createDataVector<uint64_t>()));
                    }
                    break;
                    case(ValueType::VectorFloat):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createDataVector<float>()));
                    }
                    break;
                    case(ValueType::VectorDouble):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createDataVector<double>()));
                    }
                    break;
                    case(ValueType::VectorString):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createDataVector<String>()));
                    }
                    break;
                    case(ValueType::VectorObjectId):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createObjectVector<ObjectId>()));
                    }
                    break;
                    case(ValueType::VectorDate):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createObjectVector<common::Date>()));
                    }
                    break;
                    case(ValueType::VectorDateRange):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createObjectVector<common::DateRange>()));
                    }
                    break;
                    case(ValueType::VectorTime):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createObjectVector<common::Time>()));
                    }
                    break;
                    case(ValueType::VectorDateTime):
                    {
                        HATN_CHECK_RETURN(vectorHandler(factory->createObjectVector<common::DateTime>()));
                    }
                    break;
                    case(ValueType::VectorSubunit):
                    {
                        // the concrete unit type is not known here, so the vector
                        // holds raw serialized-bytes views (see the Subunit/VectorSubunit
                        // case in parseValue above).
                        HATN_CHECK_RETURN(vectorHandler(factory->createObjectVector<SubunitBuf>()));
                    }
                    break;

                    default:
                    {
                        return du::unitError(du::UnitError::PARSE_ERROR);
                    }
                    break;
                }
            }
        }
        return OK;
    }
};

}

constexpr serialization::serializeT serialize{};
constexpr serialization::deserializeT deserialize{};

} // namespace update

HATN_DB_NAMESPACE_END

#endif // HATNUPDATESERIALIZATION_IPP
