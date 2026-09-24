/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/accountconfigformat.h
  *
  * The account-config twin of invitationformat.h's classifySharedInvitation(): "is this blob an
  * account_config_token, without committing to a real parse?" for a caller that needs to
  * classify a blob WITHOUT decrypting it or requiring a passphrase --
  *
  *   - an "the OS handed us a file, what is it?" dispatcher choosing between an invitation and an
  *     account config (see invitationformat.h's own doc comment, which already mandates
  *     invitation-first ordering because THIS format carries no magic of its own in its
  *     plaintext form -- see below);
  *   - the same in-app "Open file" / drag-drop entry points, for exactly the same reason.
  *
  * account_config_token (accountconfig.h) has no prefix/kind/version field, unlike
  * shared_invitation's prefix="HINV" -- so, UNLIKE classifySharedInvitation(), parse success
  * alone is not a discriminator: hatn dataunit deserialization skips unknown tags, so an
  * unrelated blob can deserialize into an all-unset account_config_token. Classification here
  * rests on a structural predicate instead (see accountconfigformat.cpp for exactly what is
  * checked and why), split by account_config_token::encrypted:
  *
  *   - encrypted==true: content is a crypt::CryptContainer::pack() output, which DOES carry a
  *     real magic (CryptContainerHeader::PREFIX/STREAM_PREFIX + VERSION) -- checked directly,
  *     no decryption, no passphrase needed.
  *   - encrypted==false: content deserializes as account_config, and the four fields
  *     activateaccountconfig.cpp itself hard-requires to activate a config (config_id,
  *     account_id, server_route, token) must all be present -- a blob missing any of them
  *     cannot honestly be called "an account config".
  *
  * Deliberately does NOT decrypt, does not require a passphrase, and never throws or errors:
  * malformed input is reported as "not an account config" rather than raised as an error --
  * classification is a question, not an operation that can fail, exactly like
  * classifySharedInvitationT.
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELACCOUNTCONFIGFORMAT_H
#define HATNCLIENTSERVERMODELACCOUNTCONFIGFORMAT_H

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/accountconfig.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//! Sanity bound on what can possibly be an account config blob. Matches the pre-existing UI-level
//! cap (whitemdesktop ui/uiaddaccountnode.cpp), rounded, so an OS-open/drop dispatcher handed an
//! arbitrary large file rejects it immediately instead of deserializing megabytes to find out.
constexpr const size_t MaxAccountConfigSize=512*1024;

/**
 * @brief What classifyAccountConfig() could tell about a blob.
 */
struct AccountConfigInfo
{
    //! The outer account_config_token deserialized AND its content passed the structural probe
    //! for its own encrypted/plaintext shape. False for random bytes, a truncated file, an
    //! invitation, a JPEG.
    bool isAccountConfig=false;

    //! account_config_token::encrypted. When true, content could not be inspected any further
    //! without the passphrase -- expired stays at its default in that case.
    bool encrypted=false;

    //! Only meaningful when isAccountConfig && !encrypted: account_config::valid_till is set and
    //! already in the past. Reported, never a reason to reject -- checkAccountConfig()
    //! (accountconfigparser.h) owns expiry as an activation-time concern, this is only
    //! informational for a caller that wants it before routing.
    bool expired=false;

    //! Everything a caller needs before it may route this blob as an account config. Unlike
    //! SharedInvitationInfo::usable(), there is no separate "known kind"/"supported version"
    //! axis here -- account_config carries no version field at all.
    bool usable() const noexcept
    {
        return isAccountConfig;
    }
};

/**
 * @brief Classify a candidate account-config blob without parsing (for the encrypted case) or
 * decrypting its payload.
 *
 * Never throws on malformed input and never reports an error: a blob that fails to deserialize
 * or fails the structural probe simply comes back with isAccountConfig==false.
 */
class HATN_CLIENT_SERVER_EXPORT classifyAccountConfigT
{
    public:

        AccountConfigInfo operator () (
            const char* data,
            size_t size,
            size_t maxSize=MaxAccountConfigSize
        ) const;

        AccountConfigInfo operator () (
            lib::string_view data,
            size_t maxSize=MaxAccountConfigSize
        ) const
        {
            return (*this)(data.data(),data.size(),maxSize);
        }
};
constexpr classifyAccountConfigT classifyAccountConfig{};

//! Cheapest possible question: "could this blob be an account config?" -- the first hop of a
//! file-kind dispatcher, before anything commits to a particular flow.
inline bool looksLikeAccountConfig(const char* data, size_t size)
{
    return classifyAccountConfig(data,size).isAccountConfig;
}

inline bool looksLikeAccountConfig(lib::string_view data)
{
    return looksLikeAccountConfig(data.data(),data.size());
}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELACCOUNTCONFIGFORMAT_H
