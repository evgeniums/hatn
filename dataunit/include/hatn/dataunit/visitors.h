/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file dataunit/visitors.h
  *
  * Contains visitor for data units serialization.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITVISITORS_H
#define HATNDATAUNITVISITORS_H

#include <system_error>

#include <boost/hana.hpp>

#include <hatn/common/error.h>
#include <hatn/common/result.h>
#include <hatn/common/runonscopeexit.h>

#include <hatn/validator/utils/reference_wrapper.hpp>
#include <hatn/validator/utils/foreach_if.hpp>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/datauniterror.h>
#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/wirebuf.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/ipp/wirebuf.ipp>

HATN_DATAUNIT_META_NAMESPACE_BEGIN

//! @todo Move to common lib

struct true_predicate_t
{
    template <typename T>
    constexpr bool operator()(T&&) const
    {
        return true;
    }
};
constexpr true_predicate_t true_predicate{};

HATN_DATAUNIT_META_NAMESPACE_END

HATN_DATAUNIT_NAMESPACE_BEGIN

#ifndef HDU_TEST_RELAX_MISSING_FIELD_SERIALIZING
    #define HDU_TEST_RELAX_MISSING_FIELD_SERIALIZING
#endif

//---------------------------------------------------------------

struct HATN_DATAUNIT_EXPORT visitors
{
    /**
     * @brief Serialize data unit without invokation of virtual methods.
     * @param obj Object to serialize.
     * @param wired Buffer to put data to.
     * @param top Is it top level object or subunit.
     * @return Serialized size or -1 in case of error.
     */
    template <typename UnitT, typename BufferT>
    static int serialize(const UnitT& obj, BufferT& wired, bool top=true)
    {
        return hana::eval_if(
            std::is_same<Unit,UnitT>{},
            [&](auto _)
            {
                // invoke virtual searilize() of unit object
                return _(obj).serialize(_(wired),_(top));
            },
            [&](auto _)
            {
                // invoke searilization of each field

                auto&& topLevel=_(top);
                auto&& unit=_(obj);
                auto&& buf=_(wired);

                if (topLevel)
                {
                    buf.clear();
                }

                if (!unit.wireDataKeeper().isNull())
                {
                    // use already serialized data
                    auto addedBytes=buf.append(*(unit.wireDataKeeper()));
                    return addedBytes;
                }

                // remember previous buffer size
                auto prevSize=buf.size();

                // predicate with Error
                auto predicate=[](auto&& ok)
                {
                    return ok;
                };

                // handler for each field
                auto handler=[&buf](auto&& field, auto&&)
                {
                    using fieldT=std::decay_t<decltype(field)>;

                    // skip optional fields which are not set
                    if (!field.fieldIsSet())
                    {
                        if (fieldT::fieldRequired() HDU_TEST_RELAX_MISSING_FIELD_SERIALIZING)
                        {
                            rawError(RawErrorCode::REQUIRED_FIELD_MISSING,field.fieldId(),"required field {} is not set",field.fieldName());
                            return false;
                        }
                        return true;
                    }

                    // append tag to stream
                    if (!fieldT::fieldRepeatedUnpackedProtoBuf())
                    {
                        constexpr uint32_t tag=static_cast<uint32_t>((fieldT::fieldId()<<3)|static_cast<uint32_t>(fieldT::fieldWireType()));
                        auto* buffer=buf.mainContainer();
                        bool storeTagToMeta=fieldT::fieldCanChainBlocks() && !buf.isSingleBuffer();
                        if (storeTagToMeta)
                        {
                            buf.appendUint32(tag);
                        }
                        else
                        {
                            buf.incSize(Stream<uint32_t>::packVarInt(buffer,tag));
                        }
                    }

                    if (!field.serialize(buf))
                    {
                        rawError(RawErrorCode::FIELD_SERIALIZING_FAILED,field.fieldId(),"failed to serialize field {}",field.fieldName());
                        return false;
                    }

                    return true;
                };


                auto ok=_(unit).each(predicate,true,handler);
                if (!ok)
                {
                    return -1;
                }

                // return added size
                auto addedBytes=static_cast<int>(buf.size()-prevSize);
                return addedBytes;
            }
        );
    }

    /**
     * @brief Clear data unit without invokation of virtual methods.
     * @param obj Data unit object.
     */
    template <typename UnitT>
    static void clear(UnitT& obj)
    {
        return hana::eval_if(
            std::is_same<Unit,UnitT>{},
            [&](auto _)
            {
                _(obj).clear();
            },
            [&](auto _)
            {
                _(obj).iterate([](auto& field){field.fieldClear(); return true;});
                _(obj).resetWireDataKeeper();
            }
        );
    }

    /**
     * @brief Reset data unit without invokation of virtual methods.
     * @param obj Data unit object.
     */
    template <typename UnitT>
    static void reset(UnitT& obj, bool onlyNonClean=false)
    {
        return hana::eval_if(
            std::is_same<Unit,UnitT>{},
            [&](auto _)
            {
                return _(obj).reset(_(onlyNonClean));
            },
            [&](auto _)
            {
                if (!_(onlyNonClean) || !_(obj).isClean())
                {
                    _(obj).iterate([](auto& field){field.fieldReset(); return true;});
                }
                _(obj).resetWireDataKeeper();
                _(obj).setClean(true);
            }
        );
    }

    /**
     * @brief Check if all required fields are set.
     * @param obj Unit to check.
     * @return Name of failed field or nullptr.
     */
    template <typename UnitT>
    static std::pair<int,const char*> checkRequiredFields(UnitT& obj) noexcept
    {
        return hana::eval_if(
            std::is_same<Unit,UnitT>{},
            [&](auto _)
            {
                return _(obj).checkRequiredFields();
            },
            [&](auto _)
            {
                const char* failedFieldName=nullptr;
                int failedFieldId=-1;
                _(obj).iterate(
                    [&failedFieldName,&failedFieldId](const auto& field)
                    {
                        using fieldType=std::decay_t<decltype(field)>;

                        if (!fieldType::fieldRequired())
                        {
                            return true;
                        }

                        bool ok=field.isSet();
                        if (!ok)
                        {
                            failedFieldName=fieldType::fieldName();
                            failedFieldId=fieldType::fieldId();
                        }
                        return ok;
                    }
                    );

                return std::make_pair(failedFieldId,failedFieldName);
            }
        );
    }

    /**
     * @brief Get max expected size of packed data unit without invokation of virtual methods.
     * @param obj Data unit object.
     */
    template <typename UnitT>
    static size_t size(const UnitT& obj)
    {
        return hana::eval_if(
            std::is_same<Unit,UnitT>{},
            [&](auto _)
            {
                return _(obj).maxPackedSize();
            },
            [&](auto _)
            {
                size_t acc=0;
                auto handler=[&acc](auto&& field, auto&&)
                {
                    // add tag size
                    acc+=sizeof(uint32_t);
                    // add field size
                    acc+=field.fieldSize();
                    return true;
                };
                _(obj).each(meta::true_predicate,true,handler);
                return acc;
            }
        );
    }

    template <typename UnitT>
    static void setParseToSharedArrays(UnitT& obj, bool enable, const AllocatorFactory *factory)
    {
        hana::eval_if(
            std::is_same<Unit,UnitT>{},
            [&](auto _)
            {
                _(obj).setParseToSharedArrays(_(enable),_(factory));
            },
            [&](auto _)
            {
                auto handler=[&](auto&& field, auto&&)
                {
                    field.setParseToSharedArrays(_(enable),_(factory));
                    return true;
                };
                _(obj).each(meta::true_predicate,true,handler);
            }
        );
    }

    /**
     * @brief Deserialize data unit without invokation of virtual methods.
     * @param unit Object to deserialize to.
     * @param buf Buffer containing data.
     * @param top Is it top level object or subunit.
     * @return Operation status, true if success.
     */
    template <typename UnitT, typename BufferT>
    static bool deserialize(UnitT& unit, BufferT& buf, bool top=true)
    {
        return hana::eval_if(
            std::is_same<Unit,UnitT>{},
            [&](auto _)
            {
                // invoke virtual serialize() of unit object
                return _(unit).parse(_(buf),_(top));
            },
            [&](auto _)
            {
                // invoke searilization of each field

                auto&& topLevel=_(top);
                auto&& obj=_(unit);
                auto&& wired=_(buf);

                // can deserialize only from solid buffer
                if (!wired.isSingleBuffer())
                {
                    WireBufSolid singleW(wired.toSolidWireBuf());
                    return deserialize(obj,singleW,topLevel);
                }

                reset(obj,true);
                obj.setClean(false);

                uint32_t tag=0;
                common::ByteArray* buf=wired.mainContainer();

                auto cleanup=[&obj,&wired,topLevel]()
                {
                    reset(obj);
                    if (topLevel)
                    {
                        wired.resetState();
                    }
                };

                // parse stream and load fields
                for(;;)
                {
                    // parse tag from the buffer
                    if (wired.currentOffset()>=wired.size())
                    {
                        break;
                    }
                    Assert(wired.currentOffset()<buf->size(),"Wire offset overflow");

                    auto consumed=Stream<uint32_t>::unpackVarInt(buf->data()+wired.currentOffset(),buf->size()-wired.currentOffset(),tag);
                    if (consumed<0)
                    {
                        rawError(RawErrorCode::END_OF_STREAM,"broken tag at offset ",wired.currentOffset());
                        cleanup();
                        return false;
                    }
                    if (tag==0)
                    {
                        rawError(RawErrorCode::INVALID_TAG,"null tag at offset {}",wired.currentOffset());
                        cleanup();
                        return false;
                    }
                    wired.incCurrentOffset(consumed);

                    // calc field ID and type
                    int fieldType=static_cast<int>(tag&0x7u);
                    int fieldId=tag>>3;

                    // find and process field
                    const auto* parser=obj.template fieldParser<BufferT>(fieldId);
                    if (parser!=nullptr)
                    {
                        auto fieldWireType=static_cast<int>(parser->wireType);
                        if (fieldWireType!=fieldType)
                        {
                            rawError(RawErrorCode::FIELD_TYPE_MISMATCH,parser->fieldId,"invalid wire type {} for field {}",fieldType,parser->fieldName);
                            cleanup();
                            return false;
                        }
                        if (!parser->fn(obj,wired,obj.factory()))
                        {
                            rawError(RawErrorCode::FIELD_PARSING_FAILED,parser->fieldId,"failed to parse field {} at offset {}",parser->fieldName,wired.currentOffset());
                            cleanup();
                            return false;
                        }
                    }
                    else
                    {
                        consumed=0;
                        char* data=buf->data()+wired.currentOffset();
                        switch (fieldType)
                        {
                        case (static_cast<int>(WireType::WithLength)):
                        case (static_cast<int>(WireType::VarInt)):
                        {
                            uint32_t shift=0;
                            bool moreBytesLeft=false;
                            uint32_t value=0;
                            bool overflow=false;
                            auto availableSize=buf->size()-wired.currentOffset();
                            while (StreamBase::unpackVarIntByte(data,value,availableSize,consumed,overflow,moreBytesLeft,shift,56)){}
                            if (moreBytesLeft||overflow)
                            {
                                if (overflow)
                                {
                                    rawError(RawErrorCode::END_OF_STREAM,"unexpected end of buffer at size {}",buf->size());
                                }
                                else
                                {
                                    rawError(RawErrorCode::UNTERMINATED_VARINT,fieldId,"unterminated VarInt for possible field ID {} at offset {}",fieldId,wired.currentOffset());
                                }
                                cleanup();
                                return false;
                            }
                            wired.incCurrentOffset(consumed);

                            if (fieldType==static_cast<int>(WireType::WithLength))
                            {
                                if (shift>28)
                                {
                                    // length can't be more than 32 bit word
                                    rawError(RawErrorCode::INVALID_BLOCK_LENGTH,fieldId,"invalid block length for field {} at offset {}",fieldId,wired.currentOffset());
                                    cleanup();
                                    return false;
                                }
                                wired.incCurrentOffset(value);
                                if (buf->size()<wired.currentOffset())
                                {
                                    rawError(RawErrorCode::END_OF_STREAM,fieldId,"block length overflow for field {} at offset {}",fieldId,wired.currentOffset());
                                    cleanup();
                                    return false;
                                }
                            }
                        }
                        break;

                        case (static_cast<int>(WireType::Fixed32)):
                        {
                            wired.incCurrentOffset(4);
                            if (buf->size()<wired.currentOffset())
                            {
                                rawError(RawErrorCode::END_OF_STREAM,"unexpected end of buffer at size {}",buf->size());
                                cleanup();
                                return false;
                            }
                        }
                        break;

                        case (static_cast<int>(WireType::Fixed64)):
                        {
                            wired.incCurrentOffset(8);
                            if (buf->size()<wired.currentOffset())
                            {
                                rawError(RawErrorCode::END_OF_STREAM,"unexpected end of buffer at size {}",buf->size());
                                cleanup();
                                return false;
                            }
                        }
                        break;

                        default:
                        {
                            rawError(RawErrorCode::UNKNOWN_FIELD_TYPE,"unknown field type {} at offset {}",fieldType,wired.currentOffset());
                            cleanup();
                            return false;
                        }
                        break;

                        }
                    }
                }

                // check if all required fields are set
                auto failedField=checkRequiredFields(obj);
                if (failedField.second!=nullptr)
                {
                    rawError(RawErrorCode::REQUIRED_FIELD_MISSING,failedField.first,"required field {} is not set",failedField.second);
                    cleanup();
                    return false;
                }

                if (topLevel)
                {
                    wired.resetState();
                }
                return true;
            }
        );
    }

    /**
     * @brief Serialize data unit without invokation of virtual methods.
     * @param obj Object to serialize.
     * @param buf Buffer to put data to.
     * @param error Error object to fill if serialization failed.
     * @return Serialized size or -1 in case of error.
     */
    template <typename UnitT, typename BufferT>
    static int serialize(const UnitT& obj, BufferT& buf, Error& ec)
    {
        HATN_SCOPE_GUARD(
            [&ec](){
                fillError(UnitError::SERIALIZE_ERROR,ec);
                RawError::setEnabledTL(false);
            }
        )

        RawError::setEnabledTL(true);
        return serialize(obj,buf);
    }

    /**
     * @brief Deserialize data unit without invokation of virtual methods.
     * @param obj Object to deserialize to.
     * @param buf Buffer containing data.
     * @param error Error object to fill if deserialization failed.
     * @return Operation status, true if success.
     */
    template <typename UnitT, typename BufferT>
    static bool deserialize(UnitT& obj, BufferT& buf, Error& ec)
    {
        HATN_SCOPE_GUARD(
            [&ec](){
                fillError(UnitError::PARSE_ERROR,ec);
                RawError::setEnabledTL(false);
            }
        )
        RawError::setEnabledTL(true);
        return deserialize(obj,buf);
    }

    template <typename UnitT, typename BufferT>
    static bool deserializeInline(UnitT& obj, const BufferT& buf)
    {
        WireBufSolid wbuf{buf.data(),buf.size(),true};
        return deserialize(obj,wbuf);
    }

    template <typename UnitT, typename BufferT>
    static bool deserializeInline(UnitT& obj, const BufferT& buf, Error& ec)
    {
        WireBufSolid wbuf{buf.data(),buf.size(),true};
        return deserialize(obj,wbuf,ec);
    }

    template <typename UnitT>
    static bool deserializeInline(UnitT& obj, const char* data, size_t size)
    {
        WireBufSolid wbuf{data,size,true};
        return deserialize(obj,wbuf);
    }

    template <typename UnitT>
    static bool deserializeInline(UnitT& obj, const char* data, size_t size, Error& ec)
    {
        WireBufSolid wbuf{data,size,true};
        return deserialize(obj,wbuf,ec);
    }

    template <typename UnitT, typename BufferT>
    static int serializeToBuf(const UnitT& obj, BufferT& buf, size_t offset, Error& ec)
    {
        //! @todo optimization: Implement WireBuf using external container

        WireBufSolid wbuf;
        auto size=serialize(obj,wbuf,ec);
        if (size<0)
        {
            return size;
        }

        size_t minSize=offset+wbuf.mainContainer()->size();
        if (buf.size()<minSize)
        {
            buf.resize(minSize);
        }
        memcpy(buf.data()+offset,wbuf.mainContainer()->data(),wbuf.mainContainer()->size());
        return static_cast<int>(wbuf.mainContainer()->size());
    }

    template <typename UnitT, typename BufferT>
    static int serializeToBuf(const UnitT& obj, BufferT& buf, size_t offset=0)
    {
        Error ec;
        return serializeToBuf(obj,buf,offset,ec);
    }
};

using io=visitors;

//---------------------------------------------------------------

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITVISITORS_H
