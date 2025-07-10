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
#include <hatn/dataunit/datauniterror.h>

#include <hatn/dataunit/rapidjsonstream.h>
#include <hatn/dataunit/rapidjsonsaxhandlers.h>

#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/field.h>
#include <hatn/dataunit/unit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/********************** Unit **************************/

//---------------------------------------------------------------
Unit::Unit(const AllocatorFactory *factory)
    :m_clean(true),
     m_factory(factory),
     m_jsonParseHandlers(factory->objectAllocator<JsonParseHandler>()),
     m_tree(false)
{
}

//---------------------------------------------------------------
Unit::~Unit()=default;

//---------------------------------------------------------------
void Unit::clear()
{
    iterateFields([](Field& field){field.clear(); return true;});
    resetWireDataKeeper();
}

//---------------------------------------------------------------
void Unit::reset(bool onlyNonClean)
{
    if (!onlyNonClean || !m_clean)
    {
        iterateFields([](Field& field){field.reset(); return true;});
    }
    resetWireDataKeeper();
    m_clean=true;
}

//---------------------------------------------------------------
size_t Unit::maxPackedSize() const
{
    size_t acc=0;
    iterateFieldsConst([&acc](const Field& field)
    {
        // add tag size
        acc+=sizeof(uint32_t);
        // add field size
        acc+=field.maxPackedSize();
        return true;
    });
    return acc;
}

//---------------------------------------------------------------
const Field* Unit::fieldById(int id) const
{
    // must be overriden
    std::ignore=id;
    return nullptr;
}

//---------------------------------------------------------------
Field* Unit::fieldById(int id)
{
    // must be overriden
    std::ignore=id;
    return nullptr;
}

//---------------------------------------------------------------
const Field* Unit::fieldByName(common::lib::string_view name) const
{
    // in normal units this default implementation is not used because there is overriden method
    const Field* foundField=nullptr;
    iterateFieldsConst(
                [&foundField,name](const Field& field)
                {
                    common::lib::string_view fieldName(field.name());
                    if (fieldName==name)
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
Field* Unit::fieldByName(common::lib::string_view name)
{
    auto self=const_cast<const Unit*>(this);
    auto foundField=self->fieldByName(name);
    if (foundField!=nullptr)
    {
        return const_cast<Field*>(foundField);
    }
    return nullptr;
}

//---------------------------------------------------------------
int Unit::serialize(WireData&, bool) const
{
    // must be implemented in derived class
    return -1;
}

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
int Unit::serialize(WireBufSolidRef&, bool) const
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

//---------------------------------------------------------------
int Unit::serialize(char *buf, size_t bufSize, bool checkSize) const
{
    if (checkSize)
    {
        auto expectedSize=maxPackedSize();
        if (bufSize<expectedSize)
        {
            return -1;
        }
    }

    WireDataSingle wired(buf,bufSize,true);
    return serialize(wired);
}

//---------------------------------------------------------------
void Unit::setParseToSharedArrays(bool enable, const AllocatorFactory *factory)
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
    // failed for TYPE_DATAUNIT, actual parsing must be implemented in derived class
    return false;
}

bool Unit::parse(WireBufSolid&,bool)
{
    // failed for TYPE_DATAUNIT, actual parsing must be implemented in derived class
    return false;
}

bool Unit::parse(WireBufSolidShared&,bool)
{
    // failed for TYPE_DATAUNIT, actual parsing must be implemented in derived class
    return false;
}

bool Unit::parse(WireBufChained&,bool)
{
    // failed for TYPE_DATAUNIT, actual parsing must be implemented in derived class
    return false;
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
        int maxDecimalPlaces,
        Error* ec
    ) const
{
    auto prevEcEnabled=RawError::isEnabledTL();
    if (ec!=nullptr)
    {
        RawError::setEnabledTL(true);
    }

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

    if (ec!=nullptr)
    {
        if (!ok)
        {
            fillError(UnitError::JSON_SERIALIZE_ERROR,*ec);
        }
        RawError::setEnabledTL(prevEcEnabled);
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
                    [writer](const Field& field)
                    {
                        // skip fields excluded from serialization
                        if (field.isNoSerialize())
                        {
                            return true;
                        }

                        // skip optional fields which are not set
                        if (!field.isSet() && !field.hasDefaultValue())
                        {
                            if (field.isRequired())
                            {
                                rawError(RawErrorCode::REQUIRED_FIELD_MISSING,field.getID(),"failed to serialize message to JSON: required field {} is not set",field.name());
                                return false;
                            }
                            return true;
                        }

                        writer->Key(field.name(),static_cast<json::SizeType>(strlen(field.name())));

                        // pack field
                        if (!field.toJSON(writer))
                        {
                            rawError(RawErrorCode::JSON_FIELD_SERIALIZE_ERROR,field.getID(),"failed to serialize field {} to JSON",field.name());
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
        rawError(RawErrorCode::JSON_PARSE_ERROR,"failed to parse JSON: {} at offset {}",rapidjson::GetParseError_En(reader.GetParseErrorCode()),reader.GetErrorOffset());
        clear();
    }
    return !reader.HasParseError();
}

//---------------------------------------------------------------
bool Unit::loadFromJSON(const common::lib::string_view &str, Error& ec)
{
    auto prevEcEnabled=RawError::isEnabledTL();
    RawError::setEnabledTL(true);

    auto ok=loadFromJSON(str);
    if (!ok)
    {
        fillError(UnitError::JSON_PARSE_ERROR,ec);
        RawError::setEnabledTL(prevEcEnabled);
        return ec;
    }

    RawError::setEnabledTL(prevEcEnabled);
    return ok;
}

//---------------------------------------------------------------
Error Unit::toJSONWithEc(common::ByteArray& buf,bool prettyFormat,int maxDecimalPlaces) const
{
    Error ec;
    toJSONImpl(buf,prettyFormat,maxDecimalPlaces,&ec);
    return ec;
}

//---------------------------------------------------------------
void Unit::pushJsonParseHandler(const JsonParseHandler &handler)
{
    m_jsonParseHandlers.push_back(handler);
}

//---------------------------------------------------------------
common::SharedPtr<Unit> Unit::createManagedUnit() const
{
    return common::SharedPtr<Unit>{};
}

//---------------------------------------------------------------
common::SharedPtr<Unit> Unit::toManagedUnit() const
{
    Assert(false,"A managed DataUnit can be created only by other managed unit");
    return common::SharedPtr<Unit>();
}

//---------------------------------------------------------------
bool Unit::isManagedUnit() const noexcept
{
    return false;
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
void Unit::setFieldParent(Field& field)
{
    field.setParent(this);
}

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END
