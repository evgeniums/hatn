/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/client.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENT_IPP
#define HATNAPICLIENT_IPP

#include <hatn/api/apiliberror.h>
#include <hatn/api/tenancy.h>
#include <hatn/api/client/clientrequest.h>
#include <hatn/api/client/client.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
common::Result<
        common::SharedPtr<typename Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::ReqCtx>
    >
    Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::prepare(
        const common::SharedPtr<Context>& ctx,
        SessionWrapperT session,
        const Service& service,
        const Method& method,
        MessageType message,
        lib::string_view topic,
        MethodAuth methodAuth
    )
{
    HATN_CTX_SCOPE("apiclientprepare")

    auto req=common::allocateShared<ReqCtx>(m_allocatorFactory->objectAllocator<ReqCtx>(m_thread,m_allocatorFactory,std::move(session),std::move(message),std::move(methodAuth)));
    const Tenancy& tenancy=Tenancy::contextTenancy(*ctx);
    auto ec=req->serialize(service,method,topic,tenancy);
    HATN_CTX_CHECK_EC(ec)
    return req;
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
Error Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::exec(
    common::SharedPtr<Context> ctx,
    RequestCb<Context> callback,
    SessionWrapperT session,
    const Service& service,
    const Method& method,
    MessageType message,    
    lib::string_view  topic,
    Priority priority,
    uint32_t timeoutMs,
    MethodAuth methodAuth
    )
{
    HATN_CTX_SCOPE("apiclientexec")

    auto req=common::allocateShared<ReqCtx>(m_allocatorFactory->objectAllocator<ReqCtx>(),m_thread,m_allocatorFactory,std::move(session),std::move(message),std::move(methodAuth),priority,timeoutMs);
    const Tenancy& tenancy=Tenancy::contextTenancy(*ctx);
    auto ec=req->serialize(service,method,topic,tenancy);
    HATN_CTX_CHECK_EC(ec)
    doExec(std::move(ctx),std::move(callback),std::move(req));
    return OK;
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::exec(
    common::SharedPtr<Context> ctx,
    RequestCb<Context> callback,
    common::SharedPtr<ReqCtx> req
    )
{
    HATN_CTX_SCOPE("apiclientexec")

    doExec(std::move(ctx),std::move(callback),std::move(req));
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::doExec(
        common::SharedPtr<Context> ctx,
        RequestCb<Context> callback,
        common::SharedPtr<ReqCtx> req,
        bool regenId
    )
{
    auto it=m_queues.find(req->priority());
    Assert(it!=m_queues.end(),"Unsupported API request priority");
    auto& queue=it->second;

    Error ec;

    // check if client is closed
    if (m_closed)
    {
        ec=commonError(common::CommonError::ABORTED);
    }
    else if (req->priority()!=Priority::Highest && queue.size()>(m_cfg->config().fieldValue(config::max_queue_depth)+m_sessionWaitingReqCount[req->priority()]))
    {
        // check if queue is filled
        ec=apiLibError(ApiLibError::QUEUE_OVERFLOW);
    }

    if (ec)
    {
        // report on error
        m_thread->execAsync(
            [callback{std::move(callback)},ctx{std::move(ctx)},ec{std::move(ec)}]()
            {
                ctx->onAsyncHandlerEnter();

                HATN_CTX_SCOPE("apiclientexec")

                callback(ctx,ec,{});

                ctx->onAsyncHandlerExit();
            }
        );
        return;
    }

    // set task context and callback
    if (!req->taskCtx)
    {
        req->setTaskContext(std::move(ctx));
        req->setCallback(std::move(callback));
    }

    // if session not ready then push to waiting queue for this session
    if (!req->session()->isValid())
    {
        pushToSessionWaitingQueue(std::move(req));
        return;
    }

    // regenerate request ID if needed
    if (regenId)
    {
        req->regenId();
    }

    // push request to queue
    //! @todo Use thread safe queue
    auto priority=req->priority();
    queue.push(std::move(req));

    // try to dequeue requestss
    auto clientCtx=this->sharedMainCtx();
    m_thread->execAsync(
        [priority,clientCtx{std::move(clientCtx)},this]()
        {
            std::ignore=clientCtx;
            dequeue(priority);
        }
    );
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::dequeue(Priority priority)
{
    if (m_closed)
    {
        return;
    }

    auto it=m_queues.find(priority);
    auto& queue=it->second;
    while (!queue.empty())
    {
        auto* front=queue.front();
        if ((*front)->cancelled())
        {
            queue.pop();
            continue;
        }

        if (!m_connectionPool.canSend(priority))
        {
            break;
        }

        auto req=queue.pop();
        auto taskCtx=req->taskCtx;
        taskCtx->onAsyncHandlerEnter();
        sendRequest(std::move(req));
        taskCtx->onAsyncHandlerExit();
    }
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::sendRequest(common::SharedPtr<ReqCtx> req)
{
    HATN_CTX_SCOPE("apiclientsend")

    auto clientCtx=this->sharedMainCtx();

    Error ec;
    // check if client is closed
    if (m_closed)
    {
        ec=commonError(common::CommonError::ABORTED);
    }
    else
    {
        // serialize request
        ec=req->serialize();
    }
    if (ec)
    {
        // serialization error
        m_thread->execAsync(
            [req{std::move(req)},ec{std::move(ec)},clientCtx{std::move(clientCtx)},this]()
            {
                req->taskCtx->onAsyncHandlerEnter();

                HATN_CTX_SCOPE("apiclientsend")

                req->callback(req->taskCtx,ec,{});

                req->taskCtx->onAsyncHandlerExit();

                dequeue(req->priority());
            }
        );
        return;
    }

    // send request
    auto reqPtr=req.get();
    m_connectionPool.send(
        reqPtr->taskCtx,
        reqPtr->priority(),
        reqPtr->spanBuffers(),
        [req{std::move(req)},clientCtx{std::move(clientCtx)},this](const Error& ec, auto connection)
        {
            HATN_CTX_SCOPE("apiclientsendcb")

            if (ec)
            {
                // failed to send, maybe connection is broken
                if (!req->cancelled())
                {
                    req->callback(req->taskCtx,ec,{});
                }

                // nothing to do if client is closed
                if (m_closed)
                {
                    return;
                }

                // dequeue next request
                m_thread->execAsync(
                    [priority{req->priority()},clientCtx{std::move(clientCtx)},this]()
                    {
                        std::ignore=clientCtx;
                        dequeue(priority);
                    }
                );

                return;
            }

            // receive response
            recvResponse(std::move(req),connection);
        }
    );
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
template <typename Connection>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::recvResponse(common::SharedPtr<ReqCtx> req, Connection connection)
{
    HATN_CTX_SCOPE("apiclientrecv")

    auto clientCtx=this->sharedMainCtx();
    auto reqPtr=req.get();

    m_connectionPool.recv(
        reqPtr->taskCtx,
        std::move(connection),
        reqPtr->responseData,
        [req{std::move(req)},clientCtx{std::move(clientCtx)},this](const Error& ec)
        {
            HATN_CTX_SCOPE("apiclientrecvcb")

            if (!req->cancelled())
            {
                if (ec)
                {
                    // failed to receive, maybe connection is broken
                    req->callback(req->taskCtx,ec,{});
                }
                else
                {
                    auto r=req->parseResponse();
                    if (r)
                    {
                        // parsing error
                        req->callback(req->taskCtx,r.error(),{});
                    }
                    else
                    {
                        const auto& resp=r.value();
                        const auto& respField=resp->field(protocol::response::message);
                        auto respMessage=respField.skippedNotParsedContent();
                        auto respWrapper=Response{r.takeValue(),req->responseData.sharedMainContainer(),std::move(respMessage)};
                        auto status=respWrapper.status();
                        if (!respWrapper.isSuccess())
                        {
                            if (status==protocol::ResponseStatus::AuthError && !m_closed)
                            {
                                // process auth error in session
                                refreshSession(std::move(req),std::move(respWrapper));
                            }
                            else
                            {
                                // parse error response
                                auto ec1=respWrapper.parseError(m_allocatorFactory);
                                req->callback(req->taskCtx,ec1,std::move(respWrapper));
                            }
                        }
                        else
                        {
                            // request is complete
                            req->callback(req->taskCtx,Error{},std::move(respWrapper));
                        }
                    }
                }
            }

            // dequeue next request
            m_thread->execAsync(
                [clientCtx{std::move(clientCtx)},priority{req->priority()},this]()
                {
                    std::ignore=clientCtx;
                    dequeue(priority);
                }
            );
        }
    );
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
Error Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::cancel(common::SharedPtr<ReqCtx>& req)
{
    return req->cancel();
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
template <typename ContextT1, typename CallbackT>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::close(
        common::SharedPtr<ContextT1> ctx,
        CallbackT callback,
        bool callbackRequests
    )
{
    //! @todo Use thread safe close
    m_closed=true;

    // clear queues
    for (auto&& it: m_queues)
    {
        if (callbackRequests)
        {
            while (!it.second.empty())
            {
                auto req=it.second.pop();
                if (!req->cancelled())
                {
                    req->taskCtx->onAsyncHandlerEnter();

                    HATN_CTX_SCOPE("apiclientclose")

                    req->callback(req->taskCtx,commonError(common::CommonError::ABORTED),{});

                    req->taskCtx->onAsyncHandlerExit();
                }
            }
        }
        else
        {
            it.second.clear();
        }
    }

    // reset counters
    for (auto&& it: m_sessionWaitingReqCount)
    {
        it.second=0;
    }

    // clear session waiting queues
    if (callbackRequests)
    {
        for (auto&& it: m_sessionWaitingQueues)
        {
            while (!it.second.empty())
            {
                auto req=it.second.pop();
                if (!req->cancelled())
                {
                    req->taskCtx->onAsyncHandlerEnter();

                    HATN_CTX_SCOPE("apiclientclose")

                    req->callback(req->taskCtx,commonError(common::CommonError::ABORTED),{});

                    req->taskCtx->onAsyncHandlerExit();
                }
            }
        }
    }
    m_sessionWaitingQueues.clear();

    // close connection pool
    auto clientCtx=this->sharedMainCtx();
    m_connectionPool.close(std::move(ctx),
        [clientCtx{std::move(clientCtx)},callback{std::move(callback)}](const auto& ec)
        {
            std::ignore=clientCtx;
            callback(ec);
        }
    );
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::refreshSession(common::SharedPtr<ReqCtx> req, Response resp)
{
    //! @todo Use thread safe refresh session

    HATN_CTX_SCOPE("apiclientrefreshsession")

    // set session invalid
    req->session()->setValid(false);

    // increment counter
    auto& count=m_sessionWaitingReqCount[req->priority()];
    count++;

    // find or create queue
    auto it=m_sessionWaitingQueues.find(req->session()->id());
    if (it==m_sessionWaitingQueues.end())
    {
        auto empl=m_sessionWaitingQueues.emplace(req->session()->id(),m_allocatorFactory->objectMemoryResource());
        it=empl.first;
    }

    // enqueue
    auto reqPtr=req.get();
    auto& queue=it->second;
    queue.push(std::move(req));

    // invoke session refresh
    auto clientCtx=this->sharedMainCtx();
    reqPtr->session()->refresh(
        clientCtx->id(),
        [clientCtx{std::move(clientCtx)},this](Error ec, const auto* session)
        {
            auto sessionId=session->id();
            // process in own thread
            m_thread->execAsync(
                [clientCtx{std::move(clientCtx)},this,ec{std::move(ec)},sessionId]()
                {
                    std::ignore=clientCtx;
                    if (m_closed)
                    {
                        // ignore if session is closed
                        return;
                    }

                    // find queue of waiting requests
                    auto it=m_sessionWaitingQueues.find(sessionId);
                    if (it==m_sessionWaitingQueues.end())
                    {
                       return;
                    }
                    auto& queue=it->second;

                    // handle each waiting request
                    while (!queue.empty())
                    {
                        auto req=queue.pop();

                        // decrement count
                        auto& count=m_sessionWaitingReqCount[req->priority()];
                        if (count>0)
                        {
                           count--;
                        }

                        // invoke callback on request

                        auto reqPtr=req.get();
                        auto taskCtx=reqPtr->taskCtx;
                        taskCtx->onAsyncHandlerEnter();

                        HATN_CTX_SCOPE("apiclientrefreshsession")

                        if (ec)
                        {
                           reqPtr->callback(taskCtx,ec,{});
                        }
                        else
                        {
                           doExec(taskCtx,reqPtr->callback,std::move(req),true);
                        }

                        taskCtx->onAsyncHandlerExit();
                    }

                    // delete queue for this session
                    m_sessionWaitingQueues.erase(it);
                }
            );
        },
        std::move(resp)
    );
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::pushToSessionWaitingQueue(common::SharedPtr<ReqCtx> req)
{
    //! @todo Use thread safe refresh session

    // find session queue
    auto it=m_sessionWaitingQueues.find(req->session()->id());
    if (it==m_sessionWaitingQueues.end())
    {
        // no waiting session found, call session refresh
        refreshSession(std::move(req));
        return;
    }

    // increment counter
    auto& count=m_sessionWaitingReqCount[req->priority()];
    count++;

    // just push request to waiting queue
    it->second.push(std::move(req));
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENT_IPP
