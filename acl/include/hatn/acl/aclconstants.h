/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file acl/aclconstants.h
  */

/****************************************************************************/

#ifndef HATNACLCONSTANTS_H
#define HATNACLCONSTANTS_H

#include <hatn/dataunit/objectid.h>

#include <hatn/acl/acl.h>

HATN_ACL_NAMESPACE_BEGIN

constexpr const size_t OperationNameLength=64;
constexpr const size_t ObjectNameLength=du::ObjectId::Length;
constexpr const size_t SubjectNameLength=du::ObjectId::Length;
constexpr const size_t TopicLength=du::ObjectId::Length;

using ObjectType=common::StringOnStackT<ObjectNameLength>;
using SubjectType=common::StringOnStackT<SubjectNameLength>;
using OperationType=common::StringOnStackT<OperationNameLength>;
using TopicType=common::StringOnStackT<TopicLength>;

HATN_ACL_NAMESPACE_END

#endif // HATNACLCONSTANTS_H
