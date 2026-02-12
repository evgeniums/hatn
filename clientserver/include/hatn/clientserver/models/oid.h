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
    HDU_FIELD(server_topic,TYPE_OBJECT_ID,ServerTopicFieldId)
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

struct CompareTopicObject
{
    bool operator()(const common::SharedPtr<topic_object::managed>& l,
                    const common::SharedPtr<topic_object::managed>& r) const noexcept
    {
        return HATN_DATAUNIT_NAMESPACE::unitsLess(l,r);
    }
};

struct CompareServerObject
{
    bool operator()(const common::SharedPtr<server_object::managed>& l,
                    const common::SharedPtr<server_object::managed>& r) const noexcept
    {
        return HATN_DATAUNIT_NAMESPACE::unitsLess(l,r);
    }
};

struct CompareGuid
{
    bool operator()(const common::SharedPtr<guid::managed>& l,
                    const common::SharedPtr<guid::managed>& r) const noexcept
    {
        return HATN_DATAUNIT_NAMESPACE::unitsLess(l,r);
    }
};

inline auto getUidLocal(const common::SharedPtr<uid::managed>& uid)
{
    return uid->field(uid::local).sharedValue();
}

inline auto getUidServer(const common::SharedPtr<uid::managed>& uid)
{
    return uid->field(uid::server).sharedValue();
}

inline auto getUidGlobal(const common::SharedPtr<uid::managed>& uid)
{
    return uid->field(uid::global).sharedValue();
}

template <typename PtrT>
inline std::string guidObjectHash(const PtrT& id)
{
    if (!id)
    {
        return std::string{};
    }
    auto str=fmt::format("{}-{}-{}-{}-{}",
                                id->fieldValue(guid::id),
                                id->fieldValue(guid::id_topic),
                                id->fieldValue(guid::id_type),
                                id->fieldValue(guid::issuer_id),
                                id->fieldValue(guid::issuer_schema)
                           );

    return str;
}

template <typename PtrT>
inline std::string serverObjectHash(const PtrT& id)
{
    if (!id)
    {
        return std::string{};
    }
    auto str=fmt::format("{}-{}",id->fieldValue(topic_object::oid),id->fieldValue(topic_object::topic));
    auto guidStr=guidObjectHash(id->field(server_object::server_id).sharedValue());
    if (!guidStr.empty())
    {
        str=fmt::format("{}-{}",str,guidStr);
    }
    return str;
}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSOID_H
