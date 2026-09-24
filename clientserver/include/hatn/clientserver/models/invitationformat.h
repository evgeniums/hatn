/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/invitationformat.h
  *
  * The single "is this an invitation, and what kind/version is it" answer, for every caller that
  * needs to classify a blob WITHOUT committing to parsing or decrypting it:
  *
  *   - an invitation import flow, which needs the kind before it can pick a parser and needs a
  *     precise error when the kind/version is beyond what this build handles;
  *   - a chat attachment renderer deciding whether a file row deserves an invitation-specific
  *     widget, called per row at widget-construction time, so it must stay cheap;
  *   - an "the OS handed us a file, what is it?" dispatcher choosing between an invitation and an
  *     account config (see parseAccountConfig() in clientserver/accountconfigparser.h -- that
  *     format carries NO magic of its own, so invitation-first is the only workable order).
  *
  * Deliberately does NOT decrypt, and does not touch the nested payload's bytes: it deserializes
  * the outer envelope only (a few hundred bytes in practice), reads prefix/kind/version and
  * whether a payload is present and protected, and stops. Malformed input is reported as
  * "not an invitation" rather than raised as an error -- classification is a question, not an
  * operation that can fail.
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELINVITATIONFORMAT_H
#define HATNCLIENTSERVERMODELINVITATIONFORMAT_H

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/invitation.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//! Sanity bound on what can possibly be an invitation blob. A real one is a few hundred bytes;
//! this exists so an OS-open/drop dispatcher handed an arbitrary large file rejects it
//! immediately instead of deserializing megabytes to find out.
constexpr const size_t MaxSharedInvitationSize=256*1024;

/**
 * @brief What classifySharedInvitation() could tell about a blob.
 *
 * Note the three-way split between "is an invitation at all", "is a kind this build enumerates"
 * and "is a version this build handles": all three produce different user-facing messages, and
 * collapsing them loses the ability to say "this IS an invitation, your app is just too old"
 * (the whole reason shared_invitation::prefix is not namespaced per kind).
 */
struct SharedInvitationInfo
{
    //! Envelope deserialized AND carried the expected prefix. False for anything else at all --
    //! random bytes, a truncated file, an account config, a JPEG.
    bool isInvitation=false;

    //! Raw kind/version as read off the wire; meaningless unless isInvitation. kind may be a value
    //! this build does not enumerate -- check knownKind before switching on it.
    SharedInvitationKind kind=SharedInvitationKind::Character;
    uint32_t version=1;

    //! kind maps to an enumerator this build declares (isKnownSharedInvitationKind()).
    bool knownKind=false;

    //! knownKind AND version is within maxSharedInvitationVersion(kind). False for a kind this
    //! build enumerates but whose max version is 0 (reserved, not implemented) as well as for a
    //! genuinely newer version of a supported kind -- in both cases the honest message is "update
    //! the app".
    bool supportedVersion=false;

    //! The payload field this kind uses is actually set (field 2 for Character, field 5 otherwise).
    //! Only meaningful when knownKind.
    bool payloadPresent=false;

    //! Payload is the encrypted variant, so accepting it needs the sender's protection code.
    bool protectedByCode=false;

    //! Everything a caller needs before it may attempt a real parse.
    bool usable() const noexcept
    {
        return isInvitation && knownKind && supportedVersion && payloadPresent;
    }
};

/**
 * @brief Classify a candidate invitation blob without parsing or decrypting its payload.
 *
 * Never throws on malformed input and never reports an error: a blob that fails to deserialize,
 * carries the wrong prefix, or exceeds @a maxSize simply comes back with isInvitation==false.
 */
class HATN_CLIENT_SERVER_EXPORT classifySharedInvitationT
{
    public:

        SharedInvitationInfo operator () (
            const char* data,
            size_t size,
            size_t maxSize=MaxSharedInvitationSize
        ) const;

        SharedInvitationInfo operator () (
            lib::string_view data,
            size_t maxSize=MaxSharedInvitationSize
        ) const
        {
            return (*this)(data.data(),data.size(),maxSize);
        }
};
constexpr classifySharedInvitationT classifySharedInvitation{};

//! Cheapest possible question: "could this blob be an invitation of ANY kind?" -- the first hop of
//! a file-kind dispatcher, before anything commits to a particular flow.
inline bool looksLikeSharedInvitation(const char* data, size_t size)
{
    return classifySharedInvitation(data,size).isInvitation;
}

inline bool looksLikeSharedInvitation(lib::string_view data)
{
    return looksLikeSharedInvitation(data.data(),data.size());
}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELINVITATIONFORMAT_H
