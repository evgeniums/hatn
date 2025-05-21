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
#include <hatn/utility/objectwrapper.h>

HATN_UTILITY_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ImplT, typename ContextTraits>
struct checkTopicAccessT
{
    template <typename OpHandlerT, typename ContextT, typename CallbackEcT, typename CallbackT>
    void operator () (OpHandlerT&& doOp, common::SharedPtr<ContextT> ctx, CallbackEcT callbackEc, CallbackT callback)
    {
        auto cb=[d=d,doOp=std::move(doOp),callbackEc=std::move(callbackEc),callback=std::move(callback),this](auto ctx,AccessStatus status, Error ec) mutable
        {
            if (ec || status!=AccessStatus::Grant)
            {
                if (!ec)
                {
                    ec=utilityError(UtilityError::OPERATION_FORBIDDEN);
                }
                auto cb1=[callbackEc=std::move(callbackEc),ec](auto ctx)
                {
                    callbackEc(std::move(ctx),ec);
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
    const Operation* operation;
    TopicType topic;
    lib::string_view model;
};

template <typename ImplT>
auto checkTopicAccess(std::shared_ptr<ImplT> d, const Operation* operation, lib::string_view topic, const std::string& model)
{
    return checkTopicAccessT<ImplT,typename ImplT::ContextTraits>{std::move(d),operation,topic,model};
}

template <typename ImplT, typename ContextTraits>
struct checkObjectAccessT
{
    template <typename OpHandlerT, typename ContextT,  typename CallbackEcT, typename CallbackT>
    void operator () (OpHandlerT&& doOp, common::SharedPtr<ContextT> ctx, CallbackEcT callbackEc, CallbackT callback)
    {
        auto cb=[d=d,doOp=std::move(doOp),callbackEc=std::move(callbackEc),callback=std::move(callback),this](auto ctx,AccessStatus status, Error ec) mutable
        {
            if (ec || status!=AccessStatus::Grant)
            {
                if (!ec)
                {
                    ec=utilityError(UtilityError::OPERATION_FORBIDDEN);
                }
                auto cb1=[callbackEc=std::move(callbackEc),ec](auto ctx)
                {
                    callbackEc(std::move(ctx),ec);
                };
                ContextTraits::contextJournalNotify(ctx).invoke(std::move(ctx),
                                                                cb1,
                                                                ec,
                                                                operation,
                                                                oid,
                                                                topic,
                                                                model
                                                                );
                return;
            }
            doOp(std::move(ctx),std::move(callback));
        };
        ContextTraits::contextAccessChecker(ctx).checkAccess(std::move(ctx),std::move(cb),ObjectWrapperRef{oid.string(),topic,model},operation);
    };

    checkObjectAccessT(
            std::shared_ptr<ImplT> d,
            const Operation* operation,
            const db::ObjectId& oid,
            lib::string_view topic,
            const std::string& model
        ) : d(std::move(d)),
            operation(operation),
            oid(oid),
            topic(topic),
            model(model)
    {}

    std::shared_ptr<ImplT> d;
    const Operation* operation;
    db::ObjectId oid;
    TopicType topic;
    lib::string_view model;
};

template <typename ImplT>
auto checkObjectAccess(std::shared_ptr<ImplT> d, const Operation* operation, const db::ObjectId& oid, lib::string_view topic, const std::string& model)
{
    return checkObjectAccessT<ImplT,typename ImplT::ContextTraits>{std::move(d),operation,oid,topic,model};
}


//--------------------------------------------------------------------------

HATN_UTILITY_NAMESPACE_END

#endif // HATNCHECKACCESS_H
