/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/rapidjsonsaxhandlers.h
  *
  *      Handlers for SAX rapidjson API
  *
  */

/****************************************************************************/

#ifndef HATNRAPIDJSONSAXHANDLERS_H
#define HATNRAPIDJSONSAXHANDLERS_H

#include <functional>

#ifndef RAPIDJSON_NO_SIZETYPEDEFINE
#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson { using SizeType=size_t; }
#endif

#include <rapidjson/reader.h>

#include <hatn/thirdparty/base64/base64.h>

#include <hatn/common/datetime.h>

#include <hatn/dataunit/dataunit.h>

#include <hatn/dataunit/datauniterror.h>
#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/rapidjsonstream.h>
#include <hatn/dataunit/detail/types.ipp>

HATN_DATAUNIT_NAMESPACE_BEGIN
namespace json {

using SizeType=rapidjson::SizeType;

//! Base JSON read handler
struct ReaderBase
{
    Unit* m_topUnit;
    int m_scopes;

    ReaderBase(
            Unit* topUnit,
            int scopes
        ) : m_topUnit(topUnit),
            m_scopes(scopes)
    {}
};

template <typename T,typename T1> inline void pushHandler(
            Unit* topUnit,
            T* obj,
            int scopes=0,
            int offset=0
        )
{
    auto handler=[topUnit,obj,scopes,offset](
                rapidjson::Reader& reader,
                RapidJsonStringViewStream& stream
            )
    {
        if (!reader.IterativeParseComplete())
        {
            T1 h(topUnit,obj,scopes,offset);
            return reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(stream,h);
        }
        return true;
    };
    topUnit->pushJsonParseHandler(handler);
}

template <typename T> struct ReaderBaseHandler : public ReaderBase
{
    ReaderBaseHandler(
            Unit* topUnit,
            int scopes
        ) : ReaderBase(topUnit,scopes)
    {}
};

//! JSON read handler for Unit
struct HATN_DATAUNIT_EXPORT UnitReader : public ReaderBaseHandler<UnitReader>, public rapidjson::BaseReaderHandler<rapidjson::UTF8<>,json::UnitReader>
{
    Unit* m_unit;
    int m_initialScopes;

    UnitReader(
            Unit* topUnit,
            Unit* currentUnit,
            int scopes,
            int offset
        ) : ReaderBaseHandler<UnitReader>(topUnit,scopes+offset),
            m_unit(currentUnit),
            m_initialScopes(scopes)
    {}

    bool StartObject()
    {
        pushHandler<Unit,UnitReader>(this->m_topUnit,m_unit,m_scopes);
        ++m_scopes;
        return true;
    }
    bool Key(const char* str, rapidjson::SizeType length, bool)
    {
        pushHandler<Unit,UnitReader>(this->m_topUnit,m_unit,m_scopes);

        if ((m_scopes-m_initialScopes)>1)
        {
            // in case of skeeping unknown field
            return true;
        }

        auto field=m_unit->fieldByName({str,length});
        if (field!=nullptr)
        {
            field->pushJsonParseHandler(this->m_topUnit);
        }
        else
        {
            // unknown fields are valid, just skip them
        }
        return true;
    }
    bool EndObject(rapidjson::SizeType)
    {
        return true;
    }
    bool Default()
    {
        pushHandler<Unit,UnitReader>(this->m_topUnit,m_unit,m_scopes);

        // any field is ok in nested skipped fields
        return ((m_scopes-m_initialScopes)>1);
    }
};

//! Base JSON read handler for dataunit fields
template<typename FieldType, typename Encoding = rapidjson::UTF8<>>
struct FieldReaderBase : public ReaderBaseHandler<FieldReaderBase<FieldType,Encoding>>
{
    typedef typename Encoding::Ch Ch;
    using Override=FieldReaderBase<FieldType,Encoding>;

    FieldType* m_field;
    FieldReaderBase(
            Unit* topUnit,
            FieldType* field,
            int scopes,
            int
        ) : ReaderBaseHandler<FieldReaderBase<FieldType,Encoding>>(topUnit,scopes),
            m_field(field)
    {
    }

    bool Default() {
        rawError(RawErrorCode::JSON_PARSE_ERROR,"invalid format of JSON field");
        return false;
    }
    bool Null() {
        return static_cast<Override&>(*this).Default();
    }
    bool Bool(bool) {
        return static_cast<Override&>(*this).Default();
    }
    bool Int(int) {
        return static_cast<Override&>(*this).Default();
    }
    bool Uint(unsigned) {
        return static_cast<Override&>(*this).Default();
    }
    bool Int64(int64_t) {
        return static_cast<Override&>(*this).Default();
    }
    bool Uint64(uint64_t) {
        return static_cast<Override&>(*this).Default();
    }
    bool Double(double) {
        return static_cast<Override&>(*this).Default();
    }
    /// enabled via kParseNumbersAsStringsFlag, string is not null-terminated (use length)
    bool RawNumber(const Ch* str, SizeType len, bool copy) {
        return static_cast<Override&>(*this).String(str, len, copy);
    }
    bool String(const Ch*, SizeType, bool) {
        return static_cast<Override&>(*this).Default();
    }
    bool StartObject() {
        return static_cast<Override&>(*this).Default();
    }
    bool Key(const Ch* str, SizeType len, bool copy) {
        return static_cast<Override&>(*this).String(str, len, copy);
    }
    bool EndObject(SizeType) {
        return static_cast<Override&>(*this).Default();
    }
    bool StartArray() {
        return static_cast<Override&>(*this).Default();
    }
    bool EndArray(SizeType) {
        return static_cast<Override&>(*this).Default();
    }
};


template <typename TYPE,typename FieldType,typename=void>
struct FieldReader
{
};

//! JSON read handler for integer fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                    FieldType,
                    std::enable_if_t<
                        !decltype(meta::is_custom_type<TYPE>())::value
                        &&
                        !FieldType::isRepeatedType::value
                        &&
                        std::is_integral<typename TYPE::type>::value
                    >>
                : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool Bool(bool val) { this->m_field->set(static_cast<typename TYPE::type>(val)); return true; }
    bool Int(int val) { this->m_field->set(static_cast<typename TYPE::type>(val)); return true; }
    bool Uint(unsigned val) { this->m_field->set(static_cast<typename TYPE::type>(val)); return true; }
    bool Int64(int64_t val) { this->m_field->set(static_cast<typename TYPE::type>(val)); return true; }
    bool Uint64(uint64_t val) { this->m_field->set(static_cast<typename TYPE::type>(val)); return true; }
};

//! JSON read handler for repeatable integer fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                    FieldType,
                    std::enable_if_t<
                            !decltype(meta::is_custom_type<TYPE>())::value
                            &&
                            FieldType::isRepeatedType::value
                            &&
                            std::is_integral<typename TYPE::type>::value
                        >
                     > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool StartArray()
    {
        pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);
        return true;
    }

    bool Bool(bool val) { pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);this->m_field->addValue(static_cast<typename FieldType::type>(val)); return true; }
    bool Int(int val) { pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);this->m_field->addValue(static_cast<typename FieldType::type>(val)); return true; }
    bool Uint(unsigned val) { pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);this->m_field->addValue(static_cast<typename FieldType::type>(val)); return true; }
    bool Int64(int64_t val) { pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);this->m_field->addValue(static_cast<typename FieldType::type>(val)); return true; }
    bool Uint64(uint64_t val) { pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);this->m_field->addValue(static_cast<typename FieldType::type>(val)); return true; }

    bool EndArray(SizeType)
    {
        return true;
    }
};

//! JSON read handler for floating point fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                    FieldType,
                    std::enable_if_t<
                        !decltype(meta::is_custom_type<TYPE>())::value
                        &&
                        !FieldType::isRepeatedType::value
                        &&
                        std::is_floating_point<typename TYPE::type>::value
                    >>
                : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool Bool(bool val) { this->m_field->set(static_cast<typename FieldType::type>(val)); return true; }
    bool Int(int val) { this->m_field->set(static_cast<typename FieldType::type>(val)); return true; }
    bool Uint(unsigned val) { this->m_field->set(static_cast<typename FieldType::type>(val)); return true; }
    bool Int64(int64_t val) { this->m_field->set(static_cast<typename FieldType::type>(val)); return true; }
    bool Uint64(uint64_t val) { this->m_field->set(static_cast<typename FieldType::type>(val)); return true; }
    bool Double(double val) { this->m_field->set(static_cast<typename FieldType::type>(val)); return true; }
};

//! JSON read handler for repeatable floating point fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                    FieldType,
                    std::enable_if_t<
                            !decltype(meta::is_custom_type<TYPE>())::value
                            &&
                            FieldType::isRepeatedType::value
                            &&
                            std::is_floating_point<typename FieldType::type>::value
                        >
                    > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool StartArray()
    {
        pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);
        return true;
    }

    bool Bool(bool val) { pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);this->m_field->addValue(static_cast<typename FieldType::type>(val)); return true; }
    bool Int(int val) { pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);this->m_field->addValue(static_cast<typename FieldType::type>(val)); return true; }
    bool Uint(unsigned val) { pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);this->m_field->addValue(static_cast<typename FieldType::type>(val)); return true; }
    bool Int64(int64_t val) { pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);this->m_field->addValue(static_cast<typename FieldType::type>(val)); return true; }
    bool Uint64(uint64_t val) { pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);this->m_field->addValue(static_cast<typename FieldType::type>(val)); return true; }
    bool Double(double val) { pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);this->m_field->addValue(static_cast<typename FieldType::type>(val)); return true; }

    bool EndArray(SizeType)
    {
        return true;
    }
};

//! JSON read handler for byte fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                    FieldType,
                    std::enable_if_t<
                            !decltype(meta::is_custom_type<TYPE>())::value
                            &&
                            !FieldType::isRepeatedType::value
                            &&
                            TYPE::isBytesType::value
                            &&
                            !TYPE::isStringType::value
                        >
                    > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool String(const typename FieldReaderBase<FieldType>::Ch* data, SizeType size, bool)
    {
        common::Base64::from(data,size,*this->m_field->buf());
        return true;
    }
};
//! JSON read handler for repeatable byte fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                    FieldType,
                    std::enable_if_t<
                            !decltype(meta::is_custom_type<TYPE>())::value
                            &&
                            FieldType::isRepeatedType::value
                            &&
                            TYPE::isBytesType::value
                            &&
                            !TYPE::isStringType::value
                        >
                    > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool StartArray()
    {
        pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);
        return true;
    }

    bool String(const typename FieldReaderBase<FieldType>::Ch* data, SizeType size, bool)
    {
        pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);

        auto&& val=this->m_field->createAndAddValue();
        common::Base64::from(data,size,*val.buf());
        return true;
    }

    bool EndArray(SizeType)
    {
        return true;
    }
};

//! JSON read handler for string fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                    FieldType,
                    std::enable_if_t<
                            !decltype(meta::is_custom_type<TYPE>())::value
                            &&
                            !FieldType::isRepeatedType::value
                            &&
                            TYPE::isStringType::value
                        >
                > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool String(const typename FieldReaderBase<FieldType>::Ch* data, SizeType size, bool)
    {
        this->m_field->set(data,size);
        return true;
    }
};

//! JSON read handler for repeatable string fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                    FieldType,
                    std::enable_if_t<
                            !decltype(meta::is_custom_type<TYPE>())::value
                            &&
                            FieldType::isRepeatedType::value
                            &&
                            TYPE::isStringType::value
                        >
                    > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool StartArray()
    {
        pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);
        return true;
    }

    bool String(const typename FieldReaderBase<FieldType>::Ch* data, SizeType size, bool)
    {
        pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);
        this->m_field->addValue(data,size);
        return true;
    }

    bool EndArray(SizeType)
    {
        return true;
    }
};

//! JSON read handler for dataunit fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                    FieldType,
                    std::enable_if_t<
                            !decltype(meta::is_custom_type<TYPE>())::value
                            &&
                            !FieldType::isRepeatedType::value
                            &&
                            TYPE::isUnitType::value
                        >
                    > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool Null()
    {
        return true;
    }

    bool StartObject()
    {
        pushHandler<Unit,UnitReader>(this->m_topUnit,this->m_field->mutableValue(),this->m_scopes,1);
        return true;
    }
};

//! JSON read handler for repeatable dataunit fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                FieldType,
                std::enable_if_t<
                    !decltype(meta::is_custom_type<TYPE>())::value
                    &&
                    FieldType::isRepeatedType::value
                    &&
                    TYPE::isUnitType::value
                    >
                > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool Null()
    {
        pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);
        this->m_field->createAndAddValue();
        return true;
    }

    bool StartArray()
    {
        pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);
        return true;
    }

    bool StartObject()
    {
        pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);

        auto val=this->m_field->createAndAddValue();
        pushHandler<Unit,UnitReader>(this->m_topUnit,val.mutableValue(),this->m_scopes,1);
        return true;
    }

    bool EndArray(SizeType)
    {
        return true;
    }
};

//! Base JSON write handler
class Writer
{
    public:

        using Ch=rapidjson::UTF8<>::Ch;

        Writer()=default;
        virtual ~Writer()=default;
        Writer(const Writer&)=default;
        Writer(Writer&&) =default;
        Writer& operator=(const Writer&)=default;
        Writer& operator=(Writer&&) =default;

        virtual bool Null()=0;
        virtual bool Bool(bool val)=0;
        virtual bool Int(int val)=0;
        virtual bool Uint(unsigned)=0;
        virtual bool Int64(int64_t)=0;
        virtual bool Uint64(uint64_t)=0;
        virtual bool Double(double)=0;
        virtual bool String(const Ch*, SizeType, bool=false)=0;
        virtual bool StartObject()=0;
        virtual bool Key(const Ch* str, SizeType len, bool copy=false)=0;
        virtual bool EndObject(SizeType=0)=0;
        virtual bool StartArray()=0;
        virtual bool EndArray(SizeType=0)=0;
        virtual bool RawNumber(const Ch* str, SizeType len, bool copy=false)=0;

        virtual bool RawBase64(const char *ptr, size_t size) = 0;
        virtual void SetMaxDecimalPlaces(int val) = 0;
};

//! JSON write handler
template <typename T, typename StreamT>
class WriterTmpl : public Writer
{
    public:

        WriterTmpl(StreamT& stream) : writer(stream),stream(stream)
        {}

        bool Null() override { return writer.Null(); }
        bool Bool(bool val) override { return writer.Bool(val); }
        bool Int(int val) override { return writer.Int(val); }
        bool Uint(unsigned val) override { return writer.Uint(val); }
        bool Int64(int64_t val) override { return writer.Int64(val); }
        bool Uint64(uint64_t val) override { return writer.Uint64(val); }
        bool Double(double val) override { return writer.Double(val); }
        bool RawNumber(const Ch* str, SizeType len, bool copy = false) override { return writer.RawNumber(str, len, copy); }
        bool String(const Ch* str, SizeType len, bool copy = false) override { return writer.String(str,len,copy);}
        bool StartObject() override { return writer.StartObject(); }
        bool Key(const Ch* str, SizeType len, bool copy = false) override { return writer.Key(str,len,copy); }
        bool EndObject(SizeType=0) override { return writer.EndObject(); }
        bool StartArray() override { return writer.StartArray(); }
        bool EndArray(SizeType=0) override { return writer.EndArray(); }
        bool RawBase64(const char *ptr, size_t size) override
        {
            writer.RawInlineValue(rapidjson::kStringType);
            auto len=common::Base64::encodeReserverLen(size);
            auto data=stream.reserve(len+2);
            data[0]='"';
            auto encoded=common::Base64::encode(ptr,size,data+1);
            if (len>encoded)
            {
                data=stream.rollback(len-encoded);
            }
            data[encoded+1]='"';
            return true;
        }

        void SetMaxDecimalPlaces(int val) override
        {
            if (val>0)
            {
                writer.SetMaxDecimalPlaces(val);
            }
        }

    private:

        T writer;
        StreamT& stream;
};

//! Field write handlers
template <typename T,typename=void>
struct Fieldwriter
{
    static bool write(const T&,json::Writer*)
    {
        return false;
    }
};
template <typename T>
struct Fieldwriter<T,std::enable_if_t<std::is_signed<T>::value && !std::is_floating_point<T>::value && !std::is_same<int64_t,T>::value && !std::is_same<bool,T>::value>>
{
    static bool write(const T& val,json::Writer* writer)
    {
        return writer->Int(val);
    }
};
template <typename T>
struct Fieldwriter<T,std::enable_if_t<std::is_unsigned<T>::value && !std::is_same<uint64_t,T>::value && !std::is_same<bool,T>::value>>
{
    static bool write(const T& val,json::Writer* writer)
    {
        return writer->Uint(val);
    }
};
template <typename T>
struct Fieldwriter<T,std::enable_if_t<std::is_same<uint64_t,T>::value>>
{
    static bool write(const T& val,json::Writer* writer)
    {
        return writer->Uint64(val);
    }
};
template <typename T>
struct Fieldwriter<T,std::enable_if_t<std::is_same<int64_t,T>::value>>
{
    static bool write(const T& val,json::Writer* writer)
    {
        return writer->Int64(val);
    }
};
template <typename T>
struct Fieldwriter<T,std::enable_if_t<std::is_same<bool,T>::value>>
{
    static bool write(const T& val,json::Writer* writer)
    {
        return writer->Bool(val);
    }
};
template <typename T>
struct Fieldwriter<T,std::enable_if_t<std::is_floating_point<T>::value>>
{
    static bool write(const T& val,json::Writer* writer)
    {
        return writer->Double(static_cast<double>(val));
    }
};

HATN_DATAUNIT_NAMESPACE_END
}

#endif // HATNRAPIDJSONSAXHANDLERS_H
