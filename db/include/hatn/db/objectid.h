/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/objectid.h
  *
  * Contains declaration of ObjectId.
  *
  */

/****************************************************************************/

#ifndef HATNDBOBJECTID_H
#define HATNDBOBJECTID_H

#include <atomic>
#include <chrono>

#include <hatn/common/error.h>
#include <hatn/common/databuf.h>
#include <hatn/common/datetime.h>

#include <hatn/dataunit/fields/custom.h>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

class HATN_DB_EXPORT ObjectId
{
    public:

        constexpr static const uint32_t DateTimeLength=10;
        constexpr static const uint32_t SeqLength=6;
        constexpr static const uint32_t RandLength=8;

        constexpr static const uint32_t Length=DateTimeLength+SeqLength+RandLength;

        static ObjectId generateId()
        {
            ObjectId id;
            id.generate();
            return id;
        }

        void generate();

        template <typename BufferT>
        void serialize(BufferT& buf, size_t offset=0) const
        {
            Assert((buf.size()-offset)>=Length,"invalid buf size for ObjectId");
            fmt::format_to_n(buf.data()+offset,Length,"{:010x}{:06x}{:08x}",m_timepoint,m_seq&0xFFFFFF,m_rand);
        }

        bool parse(const common::ConstDataBuf& buf) noexcept;

        std::string toString() const
        {
            std::array<char,Length> buf;
            serialize(buf);
            return std::string{buf.begin(),buf.end()};
        }

        common::DateTime toDatetime() const
        {
            return common::DateTime::fromEpochMs(m_timepoint);
        }

        uint32_t toEpoch() const noexcept
        {
            return uint32_t(m_timepoint/1000);
        }

        operator common::DateTime() const
        {
            return toDatetime();
        }

        operator common::Date() const
        {
            return toDatetime().date();
        }

        uint64_t timepoint() const noexcept
        {
            return m_timepoint;
        }

        uint32_t seq() const noexcept
        {
            return m_seq;
        }

        uint64_t rand() const noexcept
        {
            return m_rand;
        }

        bool operator ==(const ObjectId& other) const noexcept
        {
            return m_timepoint==other.m_timepoint && m_seq==other.m_seq && m_rand==other.m_rand;
        }

        bool operator !=(const ObjectId& other) const noexcept
        {
            return !(*this==other);
        }

        bool operator <(const ObjectId& other) const noexcept
        {
            if (m_timepoint<other.m_timepoint)
            {
                return true;
            }
            if (m_timepoint>other.m_timepoint)
            {
                return false;
            }
            if (m_seq<other.m_seq)
            {
                return true;
            }
            if (m_seq>other.m_seq)
            {
                return false;
            }
            if (m_rand<other.m_rand)
            {
                return true;
            }
            if (m_rand>other.m_rand)
            {
                return false;
            }
            return false;
        }

        bool operator <=(const ObjectId& other) const noexcept
        {
            return *this==other || *this<other;
        }

        bool operator >(const ObjectId& other) const noexcept
        {
            return !(*this<=other);
        }

        bool operator >=(const ObjectId& other) const noexcept
        {
            return !(*this<other);
        }

        void reset() noexcept
        {
            m_timepoint=0;
            m_seq=0;
            m_rand=0;
        }

        bool isNull() const noexcept
        {
            return m_timepoint==0;
        }

    private:

        uint64_t m_timepoint=0; // 5 bytes: 0xFFFFFFFFFF
        uint32_t m_seq=0; // 3 bytes: 0xFFFFFF
        uint32_t m_rand=0; // 4 bytes: 0xFFFFFFFF

        // hex string: 2*(5+3+4) = 24 characters
};

//! Definition of DateTime type
struct HATN_DB_EXPORT TYPE_OBJECT_ID : public HATN_DATAUNIT_NAMESPACE::types::BaseType<ObjectId,std::true_type,HATN_DATAUNIT_NAMESPACE::ValueType::Custom>
{
    using CustomType=std::true_type;
};

class ObjectIdTraits
{
    public:

        using TYPE=TYPE_OBJECT_ID;
        using type=ObjectId;

        using maxSize=std::integral_constant<int,ObjectId::Length>;

        static size_t valueSize(const ObjectId&) noexcept
        {
            return ObjectId::Length;
        }

        constexpr static size_t fieldSize() noexcept
        {
            return ObjectId::Length;
        }

        template <typename BufferT>
        static bool serialize(const ObjectId& val, BufferT& wired);

        template <typename BufferT>
        static bool deserialize(ObjectId& val, BufferT& wired, HATN_DATAUNIT_NAMESPACE::AllocatorFactory* =nullptr);

        static void fieldClear(ObjectId& value)
        {
            value.reset();
        }
};

using ObjectIdField=HATN_DATAUNIT_NAMESPACE::CustomField<ObjectIdTraits>;

HATN_DB_NAMESPACE_END

HATN_DATAUNIT_NAMESPACE_BEGIN

template <>
struct FieldTmpl<HATN_DB_NAMESPACE::TYPE_OBJECT_ID> : public HATN_DB_NAMESPACE::ObjectIdField
{
    using HATN_DB_NAMESPACE::ObjectIdField::ObjectIdField;
};

//---------------------------------------------------------------

namespace json {

//! JSON read handler for DateTime fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                   FieldType,
                   std::enable_if_t<
                       !FieldType::isRepeatedType::value
                       &&
                       std::is_same<TYPE,db::TYPE_OBJECT_ID>::value
                       >
                   > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool String(const typename FieldReaderBase<FieldType>::Ch* data, SizeType size, bool)
    {
        auto ok=this->m_field->mutableValue()->parse(common::ConstDataBuf(data,size));
        if (!ok)
        {
            this->m_field->markSet(false);
        }
        return ok;
    }
};

template <typename T>
struct Fieldwriter<T,std::enable_if_t<std::is_same<db::ObjectId,std::decay_t<T>>::value>>
{
    static bool write(const T& val,json::Writer* writer)
    {
        std::array<char,db::ObjectId::Length> buf;
        val.serialize(buf);
        return writer->String(buf.data(),buf.size());
    }
};

} // namespace json

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDBOBJECTID_H
