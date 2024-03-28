/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/unit.сpp
  *
  *      Base classes for data units and dataunit fields
  *
  */

#include <string>
#include <iostream>

#ifndef RAPIDJSON_NO_SIZETYPEDEFINE
#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson { using SizeType=size_t; }
#endif

#include <rapidjson/error/en.h>
#include <rapidjson/error/error.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

#include <hatn/common/bytearray.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/logger.h>

#include <hatn/dataunit/rapidjsonstream.h>
#include <hatn/dataunit/rapidjsonsaxhandlers.h>

#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/syntax.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

namespace {
template <typename ...Args> inline void reportDebug(const char* context,const char* msg, Args&&... args) noexcept
{
    HATN_DEBUG_CONTEXT(dataunit,context,1,HATN_FORMAT(msg,std::forward<Args>(args)...));
}
template <typename ...Args> inline void reportWarn(const char* context,const char* msg, Args&&... args) noexcept
{
    HATN_WARN_CONTEXT(dataunit,context,HATN_FORMAT(msg,std::forward<Args>(args)...));
}
}

/********************** Unit **************************/

//---------------------------------------------------------------
Unit::Unit(AllocatorFactory *factory)
    :m_clean(true),
     m_factory(factory),
     m_jsonParseHandlers(factory->objectAllocator<JsonParseHandler>())
{
}

//---------------------------------------------------------------
Unit::~Unit()=default;

//---------------------------------------------------------------
void Unit::clear()
{
    if (!m_clean)
    {
        iterateFields([](Field& field){field.clear(); return true;});
    }
    resetWireData();
    m_clean=true;
}

//---------------------------------------------------------------
void Unit::reset()
{
    if (!m_clean)
    {
        iterateFields([](Field& field){field.reset(); return true;});
    }
    resetWireData();
    m_clean=true;
}

//---------------------------------------------------------------
size_t Unit::size() const
{
    size_t acc=0;
    iterateFieldsConst([&acc](const Field& field)
    {
        // add tag size
        acc+=sizeof(uint32_t);
        // add field size
        acc+=field.size();
        return true;
    });
    return acc;
}

//---------------------------------------------------------------
const Field* Unit::fieldByName(const char* name,size_t size) const
{
    const Field* foundField=nullptr;
    iterateFieldsConst(
                [&foundField,name,size](const Field& field)
                {
                    auto sz=size;
                    if (sz==0)
                    {
                        sz=strlen(name);
                    }
                    if (field.nameSize()==sz && memcmp(field.name(),name,sz)==0)
                    {
                        foundField=&field;
                        return false;
                    }
                    return true;
                }
           );
    return foundField;
}

//---------------------------------------------------------------
Field* Unit::fieldByName(const char* name,size_t size)
{
    Field* foundField=nullptr;
    iterateFields(
                [&foundField,name,size](Field& field)
                {
                    auto sz=size;
                    if (sz==0)
                    {
                        sz=strlen(name);
                    }
                    if (field.nameSize()==sz && memcmp(field.name(),name,sz)==0)
                    {
                        foundField=&field;
                        return false;
                    }
                    return true;
                }
           );
    return foundField;
}

//---------------------------------------------------------------
void Unit::fillFieldNamesTable(common::pmr::map<FieldNamesKey, Field *> &table)
{
    iterateFields(
                [&table](Field& field)
                {
                    table[{field.name(),field.nameSize()}]=&field;
                    return true;
                }
           );
}

//---------------------------------------------------------------
int Unit::serialize(
        WireData &wired,
        bool topLevel
    ) const
{
    if (topLevel)
    {
        wired.clear();
    }

    int result=-1;
    if (!m_wireDataPack.isNull())
    {
        // use already serialized data
        result=wired.append(m_wireDataPack->wireData());
    }
    else
    {
        auto prevSize=wired.size();
        // serialize unit
        if (iterateFieldsConst(
                    [&wired,this](const Field& field)
                    {
                        // skip optional fields which are not set
                        if (!field.isSet())
                        {
                            if (field.isRequired())
                            {
                                reportWarn("serialize",
                                                    "Failed to serialize DataUnit message {}: required field {} is not set",
                                                     name(),field.name()
                                                  );
                                return false;
                            }
                            return true;
                        }

                        // append tag to stream
                        if (!field.isRepeatedUnpackedProtoBuf())
                        {
                            uint32_t tag=static_cast<uint32_t>((field.getID()<<3)|static_cast<uint32_t>(field.wireType()));
                            auto* buf=wired.mainContainer();
                            bool storeTagToMeta=
                                    !wired.isSingleBuffer()
                                    &&
                                    field.canChainBlocks()
                                ;
                            if (storeTagToMeta)
                            {
                                wired.appendUint32(tag);
                            }
                            else
                            {
                                wired.incSize(Stream<uint32_t>::packVarInt(buf,tag));
                            }
                        }

                        // pack field
                        if (!field.store(wired))
                        {
                            reportWarn("serialize","Failed to serialize DataUnit message {}: broken on field {}",
                                        name(),field.name());
                            return false;
                        }

                        // ok
                        return true;
                    }
               )
            )
        {
            result=static_cast<int>(wired.size()-prevSize);
        }
    }
    return result;
}

//---------------------------------------------------------------
int Unit::serialize(char *buf, size_t bufSize, bool checkSize) const
{
    if (checkSize)
    {
        auto expectedSize=size();
        if (bufSize<expectedSize)
        {
            return -1;
        }
    }

    WireDataSingle wired(buf,bufSize,true);
    return serialize(wired);
}

//---------------------------------------------------------------
void Unit::setParseToSharedArrays(bool enable, AllocatorFactory *factory)
{
    if (factory==nullptr)
    {
        factory=m_factory;
        if (factory==nullptr)
        {
            factory=AllocatorFactory::getDefault();
        }
    }
    iterateFields([enable,factory](Field& field){field.setParseToSharedArrays(enable,factory); return true;});
}

//---------------------------------------------------------------
bool Unit::parse(const char *data, size_t size, bool inlineBuffer)
{
    WireDataSingle wired(data,size,inlineBuffer);
    return parse(wired);
}

//---------------------------------------------------------------
bool Unit::parse(WireData &wired,bool topLevel)
{
    if (!wired.isSingleBuffer())
    {
        WireDataSingle singleW(wired.toSingleWireData());
        return parse(singleW,topLevel);
    }

    clear();
    m_clean=false;

    uint32_t tag=0;
    common::ByteArray* buf=wired.mainContainer();

    auto cleanup=[this,&wired,topLevel]()
    {
        this->clear();
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
            reportDebug("parse","Failed to parse DataUnit message {}: broken tag at offset ",name(),wired.currentOffset());
            cleanup();
            return false;
        }
        wired.incCurrentOffset(consumed);

        // calc field ID and type
        int fieldType=static_cast<int>(tag&0x7u);
        int fieldId=tag>>3;

        // find and process field
        auto* field=fieldById(fieldId);
        if (field!=nullptr)
        {
            auto fieldWireType=static_cast<int>(field->wireType());
            if (fieldWireType!=fieldType)
            {
                reportDebug("parse","Failed to parse DataUnit message {}: for field {} invalid wire type {}",name(),field->name(),fieldType);
                cleanup();
                return false;
            }
            if (!field->load(wired,m_factory))
            {
                reportDebug("parse","Failed to parse DataUnit message {}: for field {} at offset {}",name(),field->name(),wired.currentOffset());
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
                            reportDebug("parse","Failed to parse DataUnit message {}: unexpected end of buffer",name());
                        }
                        else
                        {
                            reportDebug("parse","Failed to parse DataUnit message {}: unterminated VarInt for field {} at offset ",name(),fieldId,wired.currentOffset());
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
                            reportDebug("parse","Failed to parse DataUnit message {}: invalid block length for field {} at offset ",name(),fieldId,wired.currentOffset());
                            cleanup();
                            return false;
                        }
                        wired.incCurrentOffset(value);
                        if (buf->size()<wired.currentOffset())
                        {
                            reportDebug("parse","Failed to parse DataUnit message {}: block length overflow for field {} at offset ",name(),fieldId,wired.currentOffset());
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
                        reportDebug("parse","Failed to parse DataUnit message {}: unexpected end of buffer",name());

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
                        reportDebug("parse","Failed to parse DataUnit message {}: unexpected end of buffer",name());

                        cleanup();
                        return false;
                    }
                }
                break;

                default:
                    {
                        reportDebug("parse","Failed to parse DataUnit message {}: unknown field type {} at offset {}",name(),fieldType,wired.currentOffset());
                        cleanup();
                        return false;
                    }
                    break;

            }
        }
    }

    // check if all required fields are set
    const char* failedFieldName=nullptr;
    if (!iterateFieldsConst([&failedFieldName](const Field& field)
                {
                    bool ok=!field.isRequired()||field.isSet();
                    if (!ok)
                    {
                        failedFieldName=field.name();
                    }
                    return ok;
                }
           )
        )
    {
        reportDebug("parse","Failed to parse DataUnit message {}: required field {} is not set",name(),failedFieldName);
        cleanup();
        return false;
    }

    if (topLevel)
    {
        wired.resetState();
    }
    return true;
}

template <typename T> using JsonWriter=json::WriterTmpl<
rapidjson::Writer<RapidJsonBufStream<T>>,RapidJsonBufStream<T>
>;

template <typename T> using JsonPrettyWriter=json::WriterTmpl<
rapidjson::PrettyWriter<RapidJsonBufStream<T>>,RapidJsonBufStream<T>
>;

//---------------------------------------------------------------
template <typename T>
bool Unit::toJSONImpl(
        T &buf,
        bool prettyFormat,
        int maxDecimalPlaces
    ) const
{
    bool ok=true;
    auto initialSize=buf.size();

    RapidJsonBufStream<T> stream(buf);

    json::Writer* writer;
    std::unique_ptr<JsonWriter<T>> compactWriter;
    std::unique_ptr<JsonPrettyWriter<T>> prettyWriter;
    if (prettyFormat)
    {
        prettyWriter=std::move(std::make_unique<JsonPrettyWriter<T>>(stream));
        writer=prettyWriter.get();
    }
    else
    {
        compactWriter=std::move(std::make_unique<JsonWriter<T>>(stream));
        writer=compactWriter.get();
    }
    writer->SetMaxDecimalPlaces(maxDecimalPlaces);

    ok=toJSON(writer);
    if (!ok)
    {
        buf.resize(initialSize);
    }
    return ok;
}

//---------------------------------------------------------------
bool Unit::toJSON(
        json::Writer* writer
    ) const
{
    if (writer->StartObject())
    {
        if (!iterateFieldsConst(
                    [writer,this](const Field& field)
                    {
                        // skip optional fields which are not set
                        if (!field.isSet() && !field.hasDefaultValue())
                        {
                            if (field.isRequired())
                            {
                                reportWarn("json-serialize","Failed to serialize to JSON DataUnit message {}: required field {} is not set",
                                                             name(),field.name()
                                                             );
                                return false;
                            }
                            return true;
                        }

                        writer->Key(field.name(),static_cast<json::SizeType>(strlen(field.name())));

                        // pack field
                        if (!field.toJSON(writer))
                        {
                            reportWarn("json-serialize","Failed to serialize to JSON DataUnit message {}: broken on field {}",
                                        name(),field.name());
                            return false;
                        }

                        // ok
                        return true;
                    }
               ))
        {
            return false;
        }
        return writer->EndObject();
    }
    return false;
}

//---------------------------------------------------------------
bool Unit::toJSON(common::ByteArray &buf, bool prettyFormat, int maxDecimalPlaces) const
{
    return toJSONImpl(buf,prettyFormat,maxDecimalPlaces);
}

//---------------------------------------------------------------
bool Unit::toJSON(std::string &buf, bool prettyFormat, int maxDecimalPlaces) const
{
    return toJSONImpl(buf,prettyFormat,maxDecimalPlaces);
}

//---------------------------------------------------------------
bool Unit::toJSON(common::FmtAllocatedBufferChar &buf, bool prettyFormat, int maxDecimalPlaces) const
{
    return toJSONImpl(buf,prettyFormat,maxDecimalPlaces);
}

//---------------------------------------------------------------
bool Unit::toJSON(std::vector<char> &buf, bool prettyFormat, int maxDecimalPlaces) const
{
    return toJSONImpl(buf,prettyFormat,maxDecimalPlaces);
}

//---------------------------------------------------------------
std::string Unit::toString(bool prettyFormat, int maxDecimalPlaces) const
{
    std::string str;
    common::ByteArray buf(m_factory->dataMemoryResource());
    if (toJSON(buf,prettyFormat,maxDecimalPlaces))
    {
        str=std::string(buf.data(),buf.size());
    }
    return str;
}

//---------------------------------------------------------------
bool Unit::loadFromJSON(const common::lib::string_view &str)
{
    clear();
    m_clean=false;

    rapidjson::Reader reader;
    RapidJsonStringViewStream stream(str);
    json::pushHandler<Unit,json::UnitReader>(this,this);

    reader.IterativeParseInit();

    while (!reader.IterativeParseComplete())
    {
        Assert(!m_jsonParseHandlers.empty(),"JSON handlers list can not be empty");
        auto handler=m_jsonParseHandlers.back();
        m_jsonParseHandlers.pop_back();
        if (!handler(reader,stream))
        {
            break;
        }
    }
    m_jsonParseHandlers.clear();
    if (reader.HasParseError())
    {
        reportDebug("json-parse","Failed to load from JSON: {} at offset {}",rapidjson::GetParseError_En(reader.GetParseErrorCode()),reader.GetErrorOffset());
        clear();
    }
    return !reader.HasParseError();
}

//---------------------------------------------------------------
void Unit::pushJsonParseHandler(const JsonParseHandler &handler)
{
    m_jsonParseHandlers.push_back(handler);
}

//---------------------------------------------------------------
common::SharedPtr<Unit> Unit::createManagedUnit() const
{
    Assert(false,"A managed DataUnit can be created only by other managed unit");
    return common::SharedPtr<Unit>();
}

//---------------------------------------------------------------
bool Unit::iterateFields(const Unit::FieldVisitor& visitor)
{
    std::ignore=visitor;
    return false;
}

//---------------------------------------------------------------
bool Unit::iterateFieldsConst(const Unit::FieldVisitorConst& visitor) const
{
    std::ignore=visitor;
    return false;
}

//---------------------------------------------------------------
size_t Unit::fieldCount() const noexcept
{
    return 0;
}

//---------------------------------------------------------------
const char* Unit::name() const noexcept
{
    return "unknown";
}

//---------------------------------------------------------------
const Field* Unit::fieldById(int id) const
{
    std::ignore=id;
    return nullptr;
}

//---------------------------------------------------------------
Field* Unit::fieldById(int id)
{
    std::ignore=id;
    return nullptr;
}

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END
