/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/systemutility.h
  */

/****************************************************************************/

#ifndef HATNSYSTEMUTILITY_H
#define HATNSYSTEMUTILITY_H

#include <hatn/common/allocatoronstack.h>

#include <hatn/dataunit/objectid.h>

#include <hatn/utility/utility.h>

HATN_UTILITY_NAMESPACE_BEGIN

constexpr const char* SystemTopic="system";

constexpr const size_t TopicLength=du::ObjectId::Length;
using TopicType=common::StringOnStackT<TopicLength>;

HATN_UTILITY_NAMESPACE_END

#endif // HATNSYSTEMUTILITY_H
