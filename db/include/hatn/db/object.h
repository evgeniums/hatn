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

class DbObject
{
    public:

        DbObject()=default;

        template <typename T>
        DbObject(HATN_COMMON_NAMESPACE::SharedPtr<T> sharedUnit,
                    lib::string_view topic=lib::string_view{}
                    ) :
            m_shared(sharedUnit.template staticCast<HATN_DATAUNIT_NAMESPACE::Unit>()),
            m_topic(topic)
        {}

        DbObject(HATN_COMMON_NAMESPACE::SharedPtr<HATN_DATAUNIT_NAMESPACE::Unit> sharedUnit,
                    lib::string_view topic=lib::string_view{}
                    ) :
            m_shared(std::move(sharedUnit)),
            m_topic(topic)
        {}

        template <typename T>
        T* unit()
        {
            static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::unit_tag,T>,"T must be of Unit type");

            static T sample;
            return sample.castToUnit(m_shared.get());
        }

        template <typename T>
        const T* unit() const
        {
            static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::unit_tag,T>,"T must be of Unit type");

            static T sample;
            return sample.castToUnit(m_shared.get());
        }

        template <typename T>
        T* managedUnit()
        {
            static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>,"T must be of ManagedUnit type");

            static T sample;
            return sample.castToManagedUnit(m_shared.get());
        }

        template <typename T>
        T* managedUnit() const
        {
            static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>,"T must be of ManagedUnit type");

            static T sample;
            return sample.castToManagedUnit(m_shared.get());
        }

        lib::string_view topic() const noexcept
        {
            return m_topic;
        }

        bool isNull() const noexcept
        {
            return this->m_shared.isNull();
        }

        operator bool() const noexcept
        {
            return !isNull();
        }

        template <typename T>
        T* get() const noexcept
        {
            auto self=const_cast<DbObject*>(this);
            return self->template get<T>();
        }

        template <typename T>
        T* get() noexcept
        {            
            if constexpr (hana::is_a<HATN_DATAUNIT_META_NAMESPACE::unit_tag,T>)
            {
                return unit<T>();
            }
            else if constexpr (hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>)
            {
                return managedUnit<T>();
            }
            else
            {
                static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>
                                  ||
                                  hana::is_a<HATN_DATAUNIT_META_NAMESPACE::unit_tag,T>
                              ,"T must be either of Unit or of ManagedUnit type");
                return T(nullptr);
            }
        }

        template <typename T>
        T* mutableValue() noexcept
        {
            return *get<T>();
        }

        template <typename T>
        const T& value() const noexcept
        {
            return *get<T>();
        }

        template <typename T>
        T* as() const noexcept
        {
            return get<T>();
        }

        template <typename T>
        T* as() noexcept
        {
            return get<T>();
        }

        auto shared() const noexcept
        {
            return m_shared;
        }

        HATN_DATAUNIT_NAMESPACE::Unit* get() const noexcept
        {
            return m_shared.get();
        }

    protected:

        HATN_COMMON_NAMESPACE::SharedPtr<HATN_DATAUNIT_NAMESPACE::Unit> m_shared;
        TopicContainer m_topic;
};

template <typename T>
class DbObjectT : public DbObject
{
    public:

        static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>,"T must be of ManagedUnit type");

        DbObjectT()=default;

        template <typename T1>
        DbObjectT(HATN_COMMON_NAMESPACE::SharedPtr<T1> sharedUnit,
                lib::string_view topic=lib::string_view{}
                     ) : DbObject(std::move(sharedUnit),std::move(topic))
        {
            static_assert(hana::is_a<HATN_DATAUNIT_META_NAMESPACE::managed_unit_tag,T>,"T1 must be of ManagedUnit type");
        }

        DbObjectT(HATN_COMMON_NAMESPACE::SharedPtr<HATN_DATAUNIT_NAMESPACE::Unit> sharedUnit,
                    lib::string_view topic=lib::string_view{}
                    ) : DbObject(std::move(sharedUnit),std::move(topic))
        {}

        DbObjectT(DbObject other) : DbObject(std::move(other))
        {}

        T* get() const noexcept
        {
            return this->template managedUnit<T>();
        }

        T* get() noexcept
        {
            return this->template managedUnit<T>();
        }

        T* mutableValue() noexcept
        {
            return *get();
        }

        const T& value() const noexcept
        {
            return *get();
        }

        T* operator->() const noexcept
        {
            return get();
        }

        T* operator->() noexcept
        {
            return get();
        }

        T& operator*() const noexcept
        {
            return value();
        }

        T& operator*() noexcept
        {
            return mutableValue();
        }

        //! @note Helper in case DbObjectT is used in place of DbObject
        template <typename T1>
        auto* as() const noexcept
        {
            static_assert(std::is_same<T,T1>::value,"T1 must be the same as T");
            return get();
        }

        template <typename T1>
        auto* as() noexcept
        {
            static_assert(std::is_same<T,T1>::value,"T1 must be the same as T");
            return get();
        }
};

HATN_DB_NAMESPACE_END

#endif // HATNDBOBJECT_H
