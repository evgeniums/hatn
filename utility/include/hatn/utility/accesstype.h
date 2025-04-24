/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/aclmodels.h
  */

/****************************************************************************/

#ifndef HATNUTILITYACCESSTYPE_H
#define HATNUTILITYACCESSTYPE_H

#include <utility>

#include <hatn/common/featureset.h>
#include <hatn/common/translate.h>

#include <hatn/utility/utility.h>

HATN_UTILITY_NAMESPACE_BEGIN

enum class AccessType : uint32_t
{
    Create,
    Read,
    Update,
    Delete,

    ReadOwn,
    UpdateOwn,
    DeleteOwn,

    END
};

constexpr bool isCreateAccess(AccessType accessType)
{
    return accessType==AccessType::Create;
}

constexpr bool isReadAccess(AccessType accessType)
{
    return accessType==AccessType::Read || accessType==AccessType::ReadOwn;
}

constexpr bool isUpdateAccess(AccessType accessType)
{
    return accessType==AccessType::Update || accessType==AccessType::UpdateOwn;
}

constexpr bool isDeleteAccess(AccessType accessType)
{
    return accessType==AccessType::Delete || accessType==AccessType::DeleteOwn;
}

namespace detail {

struct AccessTypeTraits
{
    using MaskType=uint32_t;
    using Feature=AccessType;
};

}

using Access=common::FeatureSet<detail::AccessTypeTraits>;
using AccessMask=Access::Features;

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYACCESSTYPE_H
