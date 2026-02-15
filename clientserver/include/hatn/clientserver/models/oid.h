/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/oid.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELSOID_H
#define HATNCLIENTSERVERMODELSOID_H

#include <hatn/common/withsharedvalue.h>

#include <hatn/db/object.h>
#include <hatn/dataunit/compare.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/encrypted.h>
#include <hatn/clientserver/models/serverroute.h>
#include <hatn/clientserver/models/revision.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const int ServerOidFieldId=110;
constexpr const int ServerTopicFieldId=111;

HDU_UNIT(oid,
    HDU_FIELD(_id,TYPE_OBJECT_ID,db::ObjectIdFieldId)
)

HDU_UNIT(at_server,
    HDU_FIELD(server_oid,TYPE_OBJECT_ID,ServerOidFieldId)
    HDU_FIELD(server_topic,TYPE_STRING,ServerTopicFieldId)
)

HDU_UNIT(oid_key,
    HDU_FIELD(oid,TYPE_OBJECT_ID,99)
)

HDU_UNIT(with_parent,
    HDU_FIELD(parent_oid,TYPE_OBJECT_ID,90)
    HDU_FIELD(parent_type,TYPE_STRING,91)
)

HDU_UNIT_WITH(topic_object,(HDU_BASE(with_parent)),
    HDU_FIELD(oid,TYPE_OBJECT_ID,1)
    HDU_FIELD(topic,TYPE_STRING,2)
)

constexpr const char* GUID_ISSUER_DNS="dns";
constexpr const char* GUID_ISSUER_PUBKEY="pubkey";
constexpr const char* GUID_ISSUER_REGISTRY="registry";
constexpr const char* GUID_ISSUER_X509="x509";
constexpr const char* GUID_ISSUER_SELF_OID="self_oid";

constexpr const char* GUID_ID_TYPE_OID="oid";
constexpr const char* GUID_ID_TYPE_USERNAME="username";
constexpr const char* GUID_ID_TYPE_ALIAS="alias";
constexpr const char* GUID_ID_TYPE_X509="x509";
constexpr const char* GUID_ID_TYPE_PUBKEY="pubkey";

HDU_UNIT(guid,
    HDU_FIELD(issuer_schema,TYPE_STRING,1,false,GUID_ISSUER_DNS)
    HDU_FIELD(issuer_id,TYPE_STRING,2)
    HDU_FIELD(id_type,TYPE_STRING,3,false,GUID_ID_TYPE_OID)
    HDU_FIELD(id,TYPE_STRING,4)
    HDU_FIELD(id_topic,TYPE_STRING,5)
)

HDU_UNIT(dns_issuer,
    HDU_FIELD(domain,TYPE_STRING,1)
    HDU_FIELD(dns_srv_name,TYPE_STRING,2)
)

HDU_UNIT(pubkey_issuer,
    HDU_FIELD(pubkey,public_key::TYPE,1)
)

HDU_UNIT(registry_issuer,
    HDU_FIELD(registry_name,TYPE_STRING,1)
    HDU_REPEATED_FIELD(servers,server_node::TYPE,2)
    HDU_REPEATED_FIELD(certificate_chain,TYPE_STRING,3)
)

HDU_UNIT(x509_issuer,
    HDU_REPEATED_FIELD(certificate_chain,TYPE_STRING,1)
)

HDU_UNIT_WITH(server_object,(HDU_BASE(topic_object)),
    HDU_FIELD(server_id,guid::TYPE,10)
)

HDU_UNIT(uid,
    HDU_FIELD(local,topic_object::TYPE,1)
    HDU_FIELD(server,server_object::TYPE,2)
    HDU_FIELD(global,guid::TYPE,3)
)

HDU_UNIT(with_uid,
    HDU_FIELD(uid,uid::TYPE,750)
)

HDU_UNIT(with_guid,
    HDU_FIELD(guid,guid::TYPE,751)
)

HDU_UNIT(with_server_id,
    HDU_FIELD(server_id,server_object::TYPE,752)
)

HDU_UNIT_WITH(global_object,(HDU_BASE(with_revision),HDU_BASE(with_uid)))

template <typename T>
inline auto getUidLocal(const T& uid)
{
    return uid->field(uid::local).sharedValue();
}

template <typename T>
inline auto getUidServer(const T& uid)
{
    return uid->field(uid::server).sharedValue();
}

template <typename T>
inline auto getUidGlobal(const T& uid)
{
    return uid->field(uid::global).sharedValue();
}

template <typename PtrT>
inline std::string guidObjectToString(const PtrT& id, const char* prefix=nullptr)
{
    if (!id)
    {
        return std::string{};
    }

    if (prefix)
    {
        return fmt::format("{}:{}|{}|{}|{}|{}",prefix,
                               id->fieldValue(guid::id),
                               id->fieldValue(guid::id_topic),
                               id->fieldValue(guid::id_type),
                               id->fieldValue(guid::issuer_id),
                               id->fieldValue(guid::issuer_schema)
                            );
    }

    return fmt::format("{}|{}|{}|{}|{}",
                           id->fieldValue(guid::id),
                           id->fieldValue(guid::id_topic),
                           id->fieldValue(guid::id_type),
                           id->fieldValue(guid::issuer_id),
                           id->fieldValue(guid::issuer_schema)
                        );
}

template <typename PtrT>
inline std::string topicObjectToString(const PtrT& id, const char* prefix=nullptr)
{
    if (!id)
    {
        return std::string{};
    }

    if (prefix)
    {
        return fmt::format("{}:{}|{}",prefix,id->fieldValue(topic_object::oid),id->fieldValue(topic_object::topic));
    }

    return fmt::format("{}|{}",id->fieldValue(topic_object::oid),id->fieldValue(topic_object::topic));
}

template <typename PtrT>
inline std::string serverObjectToString(const PtrT& id, const char* prefix=nullptr)
{
    if (!id)
    {
        return std::string{};
    }
    auto str=topicObjectToString(id);
    auto guidStr=guidObjectToString(id->field(server_object::server_id).sharedValue());
    if (prefix)
    {
        return fmt::format("{}:{}|{}",prefix,str,guidStr);
    }
    return fmt::format("{}|{}",str,guidStr);
}

class LocalUid : public common::WithSharedValue<topic_object::managed>
{
    public:

        constexpr static const char* prefix="l";
        using common::WithSharedValue<topic_object::managed>::WithSharedValue;

        const HATN_DATAUNIT_NAMESPACE::ObjectId* oid() const noexcept
        {
            if (isNull())
            {
                return nullptr;
            }
            return &value().fieldValue(topic_object::oid);
        }

        lib::string_view topic() const noexcept
        {
            if (isNull())
            {
                return lib::string_view{};
            }
            return value().fieldValue(topic_object::topic);
        }

        template <typename T>
        bool operator <(const T& other) const
        {
            return HATN_DATAUNIT_NAMESPACE::unitsLess(get(),other);
        }

        template <typename T>
        bool operator ==(const T& other) const
        {
            return HATN_DATAUNIT_NAMESPACE::unitsEqual(get(),other);
        }

        std::string toString() const
        {
            return topicObjectToString(get(),prefix);
        }
};

class Guid : public common::WithSharedValue<guid::managed>
{
    public:

        constexpr static const char* prefix="g";

        using common::WithSharedValue<guid::managed>::WithSharedValue;

        lib::string_view issuerSchema() const noexcept
        {
            if (isNull())
            {
                return lib::string_view{};
            }
            return value().fieldValue(guid::issuer_schema);
        }

        lib::string_view issuerId() const noexcept
        {
            if (isNull())
            {
                return lib::string_view{};
            }
            return value().fieldValue(guid::issuer_id);
        }

        lib::string_view id() const noexcept
        {
            if (isNull())
            {
                return lib::string_view{};
            }
            return value().fieldValue(guid::id);
        }

        lib::string_view idTopic() const noexcept
        {
            if (isNull())
            {
                return lib::string_view{};
            }
            return value().fieldValue(guid::id_topic);
        }

        lib::string_view idType() const noexcept
        {
            if (isNull())
            {
                return lib::string_view{};
            }
            return value().fieldValue(guid::id_type);
        }

        template <typename T>
        bool operator <(const T& other) const
        {
            return HATN_DATAUNIT_NAMESPACE::unitsLess(get(),other);
        }

        template <typename T>
        bool operator ==(const T& other) const
        {
            return HATN_DATAUNIT_NAMESPACE::unitsEqual(get(),other);
        }

        std::string toString() const
        {
            return guidObjectToString(get(),prefix);
        }
};

class ServerUid : public common::WithSharedValue<server_object::managed>
{
    public:

        constexpr static const char* prefix="s";
        using common::WithSharedValue<server_object::managed>::WithSharedValue;

        const HATN_DATAUNIT_NAMESPACE::ObjectId* oid() const noexcept
        {
            if (isNull())
            {
                return nullptr;
            }
            return &value().fieldValue(topic_object::oid);
        }

        lib::string_view topic() const noexcept
        {
            if (isNull())
            {
                return lib::string_view{};
            }
            return value().fieldValue(topic_object::topic);
        }

        Guid guid()
        {
            if (isNull())
            {
                return Guid{};
            }
            return value().field(server_object::server_id).sharedValue();
        }

        template <typename T>
        bool operator <(const T& other) const
        {
            return HATN_DATAUNIT_NAMESPACE::unitsLess(get(),other);
        }

        template <typename T>
        bool operator ==(const T& other) const
        {
            return HATN_DATAUNIT_NAMESPACE::unitsEqual(get(),other);
        }

        std::string toString() const
        {
            return serverObjectToString(get(),prefix);
        }
};

class Uid : public common::WithSharedValue<uid::managed>
{
    public:

        using common::WithSharedValue<uid::managed>::WithSharedValue;

        template <typename T>
        bool operator <(const T& other) const
        {
            return HATN_DATAUNIT_NAMESPACE::unitsLess(get(),other);
        }

        template <typename T>
        bool operator ==(const T& other) const
        {
            return HATN_DATAUNIT_NAMESPACE::unitsEqual(get(),other);
        }

        LocalUid local() const noexcept
        {
            if (isNull())
            {
                return LocalUid{};
            }
            return getUidLocal(get());
        }

        void setLocal(LocalUid other)
        {
            get()->field(uid::local).set(other.takeValue());
        }

        ServerUid server() const noexcept
        {
            if (isNull())
            {
                return ServerUid{};
            }
            return getUidServer(get());
        }

        void setServer(ServerUid other)
        {
            get()->field(uid::server).set(other.takeValue());
        }

        Guid global() const noexcept
        {
            if (isNull())
            {
                return Guid{};
            }
            return getUidGlobal(get());
        }

        void setGlobal(Guid other)
        {
            get()->field(uid::global).set(other.takeValue());
        }

        std::vector<std::string> ids() const
        {
            std::vector<std::string> v;
            if (local())
            {
                v.push_back(local().toString());
            }
            if (server())
            {
                v.push_back(server().toString());
            }
            if (global())
            {
                v.push_back(global().toString());
            }
            return v;
        }

        std::string toString() const
        {
            if (!isNull())
            {
                return get()->toString();
            }
            return {};
        }
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSOID_H
