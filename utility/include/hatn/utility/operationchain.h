/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/operationchain.h
  */

/****************************************************************************/

#ifndef HATNOPERATIONCHAIN_IPP
#define HATNOPERATIONCHAIN_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/utility/checkaccess.h>
#include <hatn/utility/journalnotify.h>

HATN_UTILITY_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

struct chainAclOpJournalNotifyT
{
    template <typename ImplT, typename ...Handlers>
    auto operator() (std::shared_ptr<ImplT> d,
                    const Operation* operation,
                    lib::string_view topic,
                    const std::string& model,
                    Handlers&& ...handlers) const
    {
        auto chain=hatn::chain(
            checkTopicAccess(d,operation,topic,model),
            std::forward<Handlers>(handlers)...,
            journalNotify(d,operation,topic,model)
            );
        return chain;
    }
};
constexpr chainAclOpJournalNotifyT chainAclOpJournalNotify{};

//--------------------------------------------------------------------------

struct chainAclOpJournalT
{
    template <typename ImplT, typename ...Handlers>
    auto operator() (std::shared_ptr<ImplT> d,
                    const Operation* operation,
                    const db::ObjectId& oid,
                    lib::string_view topic,
                    const std::string& model,
                    Handlers&& ...handlers) const
    {
        auto chain=hatn::chain(
            checkObjectAccess(d,operation,oid,topic,model),
            std::forward<Handlers>(handlers)...,
            journalOnly(d,operation,topic,model)
            );
        return chain;
    }

    template <typename ImplT, typename ...Handlers>
    auto operator() (std::shared_ptr<ImplT> d,
                    const Operation* operation,
                    lib::string_view topic,
                    const std::string& model,
                    Handlers&& ...handlers) const
    {
        auto chain=hatn::chain(
            checkTopicAccess(d,operation,topic,model),
            std::forward<Handlers>(handlers)...,
            journalOnly(d,operation,topic,model)
            );
        return chain;
    }
};
constexpr chainAclOpJournalT chainAclOpJournal{};

//--------------------------------------------------------------------------

HATN_UTILITY_NAMESPACE_END

#endif // HATNOPERATIONCHAIN_IPP
