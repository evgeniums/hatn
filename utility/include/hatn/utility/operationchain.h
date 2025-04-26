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

struct operationChainT
{
    template <typename ImplT, typename ...Handlers>
    auto operator() (std::shared_ptr<ImplT> d, const std::string& model, const Operation* operation, db::Topic topic, Handlers&& ...handlers) const
    {
        auto chain=hatn::chain(
            checkTopicAccess(d,model,operation,topic),
            std::forward<Handlers>(handlers)...,
            journalNotify(d,model,operation,topic)
            );
        return chain;
    }
};
constexpr operationChainT operationChain{};

//--------------------------------------------------------------------------

HATN_UTILITY_NAMESPACE_END

#endif // HATNOPERATIONCHAIN_IPP
