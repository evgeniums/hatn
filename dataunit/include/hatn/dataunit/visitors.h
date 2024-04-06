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

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/wirebuf.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/detail/wirebuf.ipp>

HATN_DATAUNIT_NAMESPACE_BEGIN

//---------------------------------------------------------------

namespace meta {
struct true_predicate_t
{
    template <typename T>
    constexpr bool operator()(T&&) const
    {
        return true;
    }
};
constexpr true_predicate_t true_predicate{};
}

struct HATN_DATAUNIT_EXPORT visitors
{

    //! @todo Change error processing.
    template <typename ...Args>
    static void reportDebug(const char* context,const char* msg, Args&&... args) noexcept
    {
        HATN_DEBUG_CONTEXT(dataunit,context,1,HATN_FORMAT(msg,std::forward<Args>(args)...));
    }
    template <typename ...Args>
    static void reportWarn(const char* context,const char* msg, Args&&... args) noexcept
    {
        HATN_WARN_CONTEXT(dataunit,context,HATN_FORMAT(msg,std::forward<Args>(args)...));
    }

    /**
     * @brief Serialize data unit without invokation of virtual methods.
     * @param obj Object to serialize.
     * @param wired Buffer to put data to.
     * @param top Is it top level object or subunit.
     * @return Serialized size or -1 in case of error.
     *
     * @todo Use descriptive error or maybe just throw on missing required fields?
     */
    template <typename UnitT, typename BufferT>
    int static serialize(const UnitT& obj, BufferT& wired, bool top=true)
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
                        if (fieldT::fieldRequired())
                        {
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
    static const char* checkRequiredFields(UnitT& obj)
    {
        const char* failedFieldName=nullptr;
        if (!obj.iterate(
                    [&failedFieldName](const auto& field)
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
                        }
                        return ok;
                    }
                )
            )
        {
            return failedFieldName;
        }

        return nullptr;
    }

    /**
     * @brief Get size of data unit without invokation of virtual methods.
     * @param obj Data unit object.
     */
    template <typename UnitT>
    static size_t size(const UnitT& obj)
    {
        return hana::eval_if(
            std::is_same<Unit,UnitT>{},
            [&](auto _)
            {
                return _(obj).size();
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
    static void fillFieldNamesTable(UnitT& obj, common::pmr::map<FieldNamesKey, Field *> &table)
    {
        hana::eval_if(
            std::is_same<Unit,UnitT>{},
            [&](auto _)
            {
                _(obj).fillFieldNamesTable(_(table));
            },
            [&](auto _)
            {
                auto& t=_(table);
                auto handler=[&t](auto&& field, auto&&)
                {
                    t[{field.fieldName(),field.fieldNameSize()}]=&field;
                    return true;
                };
                _(obj).each(meta::true_predicate,true,handler);
            }
        );
    }

    template <typename UnitT>
    static void setParseToSharedArrays(UnitT& obj, bool enable, AllocatorFactory *factory)
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
     * @param obj Buffer containing data.
     * @param top Is it top level object or subunit.
     * @return Serialized size or -1 in case of error.
     *
     * @todo Use descriptive errors.
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
                        reportDebug("parse","Failed to parse DataUnit message {}: broken tag at offset ",obj.name(),wired.currentOffset());
                        cleanup();
                        return false;
                    }
                    if (tag==0)
                    {
                        reportDebug("parse","Failed to parse DataUnit message {}: null tag at offset ",obj.name(),wired.currentOffset());
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
                            reportDebug("parse","Failed to parse DataUnit message {}: for field {} invalid wire type {}",obj.name(),parser->fieldName,fieldType);
                            cleanup();
                            return false;
                        }
                        if (!parser->fn(obj,wired,obj.factory()))
                        {
                            reportDebug("parse","Failed to parse DataUnit message {}: for field {} at offset {}",obj.name(),parser->fieldName,wired.currentOffset());
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
                                    reportDebug("parse","Failed to parse DataUnit message {}: unexpected end of buffer",obj.name());
                                }
                                else
                                {
                                    reportDebug("parse","Failed to parse DataUnit message {}: unterminated VarInt for field {} at offset ",obj.name(),fieldId,wired.currentOffset());
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
                                    reportDebug("parse","Failed to parse DataUnit message {}: invalid block length for field {} at offset ",obj.name(),fieldId,wired.currentOffset());
                                    cleanup();
                                    return false;
                                }
                                wired.incCurrentOffset(value);
                                if (buf->size()<wired.currentOffset())
                                {
                                    reportDebug("parse","Failed to parse DataUnit message {}: block length overflow for field {} at offset ",obj.name(),fieldId,wired.currentOffset());
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
                                reportDebug("parse","Failed to parse DataUnit message {}: unexpected end of buffer",obj.name());

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
                                reportDebug("parse","Failed to parse DataUnit message {}: unexpected end of buffer",obj.name());

                                cleanup();
                                return false;
                            }
                        }
                        break;

                        default:
                        {
                            reportDebug("parse","Failed to parse DataUnit message {}: unknown field type {} at offset {}",obj.name(),fieldType,wired.currentOffset());
                            cleanup();
                            return false;
                        }
                        break;

                        }
                    }
                }

                // check if all required fields are set
                const char* failedFieldName=checkRequiredFields(obj);
                if (failedFieldName!=nullptr)
                {
                    reportDebug("parse","Failed to parse DataUnit message {}: required field {} is not set",obj.name(),failedFieldName);
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
};

using io=visitors;

//---------------------------------------------------------------

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITVISITORS_H
