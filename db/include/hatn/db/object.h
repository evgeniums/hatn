/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/object.h
  *
  * Contains definition of base db object.
  *
  */

/****************************************************************************/

#ifndef HATNDBOBJECT_H
#define HATNDBOBJECT_H

#include <hatn/common/error.h>
#include <hatn/common/databuf.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/allocatoronstack.h>

#include <hatn/dataunit/unitwrapper.h>
#include <hatn/dataunit/syntax.h>

#include <hatn/db/db.h>
#include <hatn/db/objectid.h>

HATN_DB_NAMESPACE_BEGIN

constexpr const int ObjectIdFieldId=100;
constexpr const int CreatedAtFieldId=101;
constexpr const int UpdatedAtFieldId=102;

HDU_UNIT(object,
    HDU_FIELD(_id,TYPE_OBJECT_ID,ObjectIdFieldId)
    HDU_FIELD(created_at,TYPE_DATETIME,CreatedAtFieldId)
    HDU_FIELD(updated_at,TYPE_DATETIME,UpdatedAtFieldId)
)

constexpr inline const auto& Oid=object::_id;

inline std::string ObjectIdFieldName{object::_id.name()};
inline std::string CreatedAtFieldName{object::created_at.name()};
inline std::string UpdatedAtFieldName{object::updated_at.name()};

template <typename ObjectT>
void initObject(ObjectT& obj)
{
    // generate _id
    auto* id=obj.field(object::_id).mutableValue();
    id->generate();

    // set creation time
    auto dt=id->toDatetime();
    obj.field(object::created_at).set(dt);

    // set update time
    obj.field(object::updated_at).set(dt);

    // reset wire data keeper
    obj.resetWireDataKeeper();
}

template <typename ObjectT>
ObjectT makeInitObject()
{
    ObjectT obj;
    initObject(obj);
    return obj;
}

template <typename ObjectT>
auto makeInitObjectPtr()
{
    auto obj=common::makeShared<ObjectT>();
    initObject(*obj);
    return obj;
}

constexpr const size_t ReservedTopicLength=ObjectId::Length;

using TopicContainer=HATN_COMMON_NAMESPACE::StringOnStackT<ReservedTopicLength>;

class DbObject : public du::UnitWrapper
{
    public:

        DbObject()=default;

        template <typename T>
        DbObject(HATN_COMMON_NAMESPACE::SharedPtr<T> sharedUnit,
                    lib::string_view topic=lib::string_view{}
                    ) : UnitWrapper(std::move(sharedUnit)),
                        m_topic(topic)
        {}

        DbObject(HATN_COMMON_NAMESPACE::SharedPtr<HATN_DATAUNIT_NAMESPACE::Unit> sharedUnit,
                    lib::string_view topic=lib::string_view{}
                    ) :
            UnitWrapper(std::move(sharedUnit)),
            m_topic(topic)
        {}

        lib::string_view topic() const noexcept
        {
            return m_topic;
        }

    private:

        TopicContainer m_topic;
};

template <typename T>
class DbObjectT : public du::UnitWrapperT<T>
{
    public:

        using Base=du::UnitWrapperT<T>;

        DbObjectT()=default;

        template <typename T1>
        DbObjectT(HATN_COMMON_NAMESPACE::SharedPtr<T1> sharedUnit,
                    lib::string_view topic=lib::string_view{}
                ) : Base(std::move(sharedUnit)),
                    m_topic(topic)
        {}

        DbObjectT(HATN_COMMON_NAMESPACE::SharedPtr<HATN_DATAUNIT_NAMESPACE::Unit> sharedUnit,
                    lib::string_view topic=lib::string_view{}
                ) : Base(std::move(sharedUnit)),
                    m_topic(topic)
        {}

        DbObjectT(DbObject other) : Base(std::move(other))
        {}

        lib::string_view topic() const noexcept
        {
            return m_topic;
        }

    private:

        TopicContainer m_topic;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBOBJECT_H
