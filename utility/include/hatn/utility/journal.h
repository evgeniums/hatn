/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/journal.h
  */

/****************************************************************************/

#ifndef HATNUTILITYJOURNAL_H
#define HATNUTILITYJOURNAL_H

#include <hatn/common/objecttraits.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/allocatoronstack.h>

#include <hatn/dataunit/objectid.h>

#include <hatn/db/topic.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/operation.h>

HATN_UTILITY_NAMESPACE_BEGIN

constexpr const size_t PreallocatedParametersSize=4;

struct Parameter
{
    const char* name;
    std::string value;
};

template <typename ContextTraits, typename Traits>
class Journal : public common::WithTraits<Traits>
{
    public:

        using Context=typename ContextTraits::Context;

        using common::WithTraits<Traits>::WithTraits;

        template <typename CallbackT>
        void log(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const Error& status,
            const Operation* op,
            const du::ObjectId& objectId,
            const db::Topic& objectTopic,
            const std::string& objectModel,
            common::pmr::vector<Parameter> params={}
        )
        {
            this->traits(std::move(ctx),std::move(callback),status,op,objectId,objectTopic,objectModel,std::move(params));
        }
};

class JournalNone
{
    public:

        template <typename ContextT, typename CallbackT>
        void log(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const Error& /*status*/,
            const Operation* /*op*/,
            const du::ObjectId& /*objectId*/,
            const db::Topic& /*objectTopic*/,
            const std::string& /*objectModel*/,
            common::pmr::vector<Parameter> /*params*/={}
            )
        {
            callback(std::move(ctx));
        }
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYJOURNAL_H
