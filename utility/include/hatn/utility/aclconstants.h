/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/aclconstants.h
  */

/****************************************************************************/

#ifndef HATNUTILITYACLCONSTANTS_H
#define HATNUTILITYACLCONSTANTS_H

#include <hatn/dataunit/objectid.h>

#include <hatn/utility/utility.h>

HATN_UTILITY_NAMESPACE_BEGIN

constexpr const size_t OperationNameLength=64;
constexpr const size_t ObjectNameLength=du::ObjectId::Length;
constexpr const size_t SubjectNameLength=du::ObjectId::Length;

using ObjectType=common::StringOnStackT<ObjectNameLength>;
using SubjectType=common::StringOnStackT<SubjectNameLength>;

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYACLCONSTANTS_H
