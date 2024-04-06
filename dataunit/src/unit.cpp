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
  * Defines Unit class.
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

#include <hatn/dataunit/wiredata.h>

#include <hatn/dataunit/rapidjsonstream.h>
#include <hatn/dataunit/rapidjsonsaxhandlers.h>

#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/detail/wirebuf.ipp>
#include <hatn/dataunit/unit.h>

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
//! @todo Make it in visitors
void Unit::clear(bool onlyNonClean)
{
    if (!onlyNonClean || !m_clean)
    {
        iterateFields([](Field& field){field.clear(); return true;});
    }
    resetWireDataKeeper();
    m_clean=true;
}

//---------------------------------------------------------------
void Unit::reset()
{
    iterateFields([](Field& field){field.reset(); return true;});
    resetWireDataKeeper();
}

//---------------------------------------------------------------
//! @todo Make it in visitors
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
//! @todo Make it in visitors
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
//! @todo Make it in visitors
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
//! @todo Make it in visitors
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
int Unit::serialize(WireData&, bool) const
{
    // must be implemented in derived class
    return -1;
}

#if 0
//---------------------------------------------------------------
int Unit::serialize(WireBufSolid&, bool) const
{
    // must be implemented in derived class
    return -1;
}

//---------------------------------------------------------------
int Unit::serialize(WireBufSolidShared&, bool) const
{
    // must be implemented in derived class
    return -1;
}

//---------------------------------------------------------------
int Unit::serialize(WireBufChained&, bool) const
{
    // must be implemented in derived class
    return -1;
}
#endif

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
//! @todo Make it in visitors
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

bool Unit::parse(WireData&,bool)
{
    // must be implemented in derived class
    return false;
}

bool Unit::parse(WireBufSolid&,bool)
{
    // must be implemented in derived class
    return false;
}

#if 0
bool Unit::parse(WireBufSolidShared&,bool)
{
    // must be implemented in derived class
    return false;
}
#endif

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
//! @todo Make it in visitors
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
