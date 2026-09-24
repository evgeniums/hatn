/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/invitationformat.сpp
  *
  */

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/invitationformat.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

SharedInvitationInfo classifySharedInvitationT::operator () (
        const char* data,
        size_t size,
        size_t maxSize
    ) const
{
    SharedInvitationInfo info;

    if (data==nullptr || size==0 || size>maxSize)
    {
        return info;
    }

    // Envelope only. The nested payload is never touched here: `invitation`/`payload` are
    // encryptable_object, whose `plain` member is a TYPE_DATAUNIT the deserializer cannot resolve
    // to a concrete unit and therefore parks in a skip buffer -- which still marks the field set,
    // so presence checks below are accurate without any shared-arrays parse mode.
    shared_invitation::type unit;
    Error ec;
    if (!du::io::deserializeInline(unit,data,size,ec))
    {
        // Not an invitation, not an error: classification is a question, see this type's own doc
        // comment (invitationformat.h).
        return info;
    }

    if (unit.fieldValue(shared_invitation::prefix)!=InvitationPrefix)
    {
        return info;
    }
    info.isInvitation=true;

    info.kind=unit.fieldValue(shared_invitation::kind);

    // An explicit 0 is not a legal version -- absent already means 1 via the field default, so
    // normalize rather than reject, matching how the chat-message side treats msg_type_version.
    auto rawVersion=unit.fieldValue(shared_invitation::version);
    info.version=rawVersion==0 ? 1 : rawVersion;

    info.knownKind=isKnownSharedInvitationKind(info.kind);
    if (!info.knownKind)
    {
        // kind is beyond this build's vocabulary: nothing below can be interpreted, and
        // supportedVersion/payloadPresent stay false so usable() is false.
        return info;
    }

    auto maxVersion=maxSharedInvitationVersion(info.kind);
    info.supportedVersion=maxVersion!=0 && info.version<=maxVersion;

    // Character keeps the original field 2; every other kind uses field 5 -- see
    // shared_invitation's own doc comment on why the two are deliberately not merged. Written as
    // two branches rather than a ternary on purpose: each HDU field is its own distinct type, so
    // the two have no common type to select between.
    if (info.kind==SharedInvitationKind::Character)
    {
        const auto& payloadField=unit.field(shared_invitation::invitation);
        info.payloadPresent=payloadField.isSet();
        if (info.payloadPresent)
        {
            info.protectedByCode=payloadField.field(encryptable_object::encrypted).isSet();
        }
    }
    else
    {
        const auto& payloadField=unit.field(shared_invitation::payload);
        info.payloadPresent=payloadField.isSet();
        if (info.payloadPresent)
        {
            info.protectedByCode=payloadField.field(encryptable_object::encrypted).isSet();
        }
    }

    return info;
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END
