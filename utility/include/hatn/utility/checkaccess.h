/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/checkaccess.h
  */

/****************************************************************************/

#ifndef HATNCHECKACCESS_H
#define HATNCHECKACCESS_H

#include <hatn/common/meta/chain.h>

#include <hatn/utility/utilityerror.h>
#include <hatn/utility/accessstatus.h>
#include <hatn/utility/operation.h>
#include <hatn/utility/systemsection.h>

HATN_UTILITY_NAMESPACE_BEGIN

    //--------------------------------------------------------------------------

template <typename ImplT, typename ContextTraits>
struct checkTopicAccessT
{
    template <typename OpHandlerT, typename ContextT, typename CallbackT>
    void operator () (OpHandlerT&& doOp, common::SharedPtr<ContextT> ctx, CallbackT callback)
    {
        auto cb=[d=d,doOp=std::move(doOp),callback=std::move(callback),this](auto ctx,AccessStatus status, Error ec) mutable
        {
            if (ec || status!=AccessStatus::Grant)
            {
                if (!ec)
                {
                    ec=utilityError(UtilityError::OPERATION_FORBIDDEN);
                }
                auto cb1=[callback=std::move(callback),ec,topic=TopicType{topic}](auto ctx)
                {
                    callback(std::move(ctx),ec,du::ObjectId{});
                };
                ContextTraits::contextJournalNotify(ctx).invoke(std::move(ctx),
                                                                cb1,
                                                                ec,
                                                                operation,
                                                                du::ObjectId{},
                                                                topic,
                                                                model
                                                                );
                return;
            }
            doOp(std::move(ctx),std::move(callback));
        };
        ContextTraits::contextAccessChecker(ctx).checkAccess(std::move(ctx),std::move(cb),operation,topic);
    };

    std::shared_ptr<ImplT> d;
    const std::string& model;
    const Operation* operation;
    TopicType topic;
};

template <typename ImplT>
auto checkTopicAccess(std::shared_ptr<ImplT> d, const std::string& model, const Operation* operation,lib::string_view topic)
{
    return checkTopicAccessT<ImplT,typename ImplT::ContextTraits>{std::move(d),model,operation,topic};
}

//--------------------------------------------------------------------------

HATN_UTILITY_NAMESPACE_END

#endif // HATNCHECKACCESS_H
