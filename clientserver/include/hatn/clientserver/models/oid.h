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

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/encrypted.h>
#include <hatn/clientserver/models/serverroute.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const int ServerOidFieldId=110;

HDU_UNIT(oid,
    HDU_FIELD(_id,TYPE_OBJECT_ID,db::ObjectIdFieldId)
)

HDU_UNIT(at_server,
    HDU_FIELD(server_oid,TYPE_OBJECT_ID,ServerOidFieldId)
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
    HDU_FIELD(topic_guid_hashed,TYPE_STRING,3)
)

constexpr const char* GUID_ISSUER_DNS="dns";
constexpr const char* GUID_ISSUER_PUBKEY="pubkey";
constexpr const char* GUID_ISSUER_REGISTRY="registry";
constexpr const char* GUID_ISSUER_X509="x509";

constexpr const char* GUID_ID_TYPE_OID="oid";
constexpr const char* GUID_ID_TYPE_USERNAME="username";
constexpr const char* GUID_ID_TYPE_ALIAS="alias";
constexpr const char* GUID_ID_TYPE_X509="x509";
constexpr const char* GUID_ID_TYPE_PUBKEY="pubkey";

constexpr const char* GUID_LOOKUP_SCHEMA_IMPLIED="implied";
constexpr const char* GUID_LOOKUP_SCHEMA_EXPLICIT_ROUTE="explicit_route";
constexpr const char* GUID_LOOKUP_SCHEMA_BLOCKCHAIN_ETH="eth";

HDU_UNIT(guid,
    HDU_FIELD(issuer_schema,TYPE_STRING,1)
    HDU_FIELD(issuer,TYPE_DATAUNIT,2)
    HDU_FIELD(id_type,TYPE_STRING,3)
    HDU_FIELD(id,TYPE_STRING,4)
    HDU_FIELD(id_topic,TYPE_STRING,5)
    HDU_FIELD(lookup_schema,TYPE_STRING,6)
    HDU_FIELD(lookup_content,TYPE_DATAUNIT,7)
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

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSOID_H
