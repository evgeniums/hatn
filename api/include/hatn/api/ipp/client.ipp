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

#include <hatn/api/apierror.h>
#include <hatn/api/tenancy.h>
#include <hatn/api/client/request.h>
#include <hatn/api/client/client.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename MessgaeBufT, typename RequestUnitT>
common::Result<
        common::SharedPtr<typename Client<RouterTraits,SessionTraits,ContextT,MessgaeBufT,RequestUnitT>::ReqCtx>
    >
    Client<RouterTraits,SessionTraits,ContextT,MessgaeBufT,RequestUnitT>::prepare(
        const common::SharedPtr<Context>& ctx,
        common::SharedPtr<Session<SessionTraits>> session,
        const Service& service,
        const Method& method,
        MessageType message,
        MethodAuth methodAuth
    )
{
    HATN_CTX_SCOPE("apiclientprepare")

    auto req=common::allocateShared<ReqCtx>(m_allocatorFactory->objectAllocator<ReqCtx>(m_thread,m_allocatorFactory,std::move(session),std::move(message),std::move(methodAuth)));
    const Tenancy& tenancy=Tenancy::contextTenancy(*ctx);
    auto ec=req->serialize(service,method,std::move(message),tenancy);
    HATN_CTX_CHECK_EC(ec)
    return req;
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename MessgaeBufT, typename RequestUnitT>
Error Client<RouterTraits,SessionTraits,ContextT,MessgaeBufT,RequestUnitT>::exec(
    common::SharedPtr<Context> ctx,
    common::SharedPtr<Session<SessionTraits>> session,
    const Service& service,
    const Method& method,
    MessageType message,
    RequestCb<Context> callback,
    lib::string_view  topic,
    Priority priority,
    uint32_t timeoutMs,
    MethodAuth methodAuth
    )
{
    HATN_CTX_SCOPE("apiclientexec")

    auto req=common::allocateShared<ReqCtx>(m_allocatorFactory->objectAllocator<ReqCtx>(m_thread,m_allocatorFactory,std::move(session),std::move(message),std::move(methodAuth),priority,timeoutMs));
    const Tenancy& tenancy=Tenancy::contextTenancy(*ctx);
    auto ec=req->serialize(service,method,std::move(message),topic,tenancy);
    HATN_CTX_CHECK_EC(ec)
    doExec(std::move(ctx),std::move(req),std::move(callback));
    return OK;
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename MessgaeBufT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,MessgaeBufT,RequestUnitT>::exec(
    common::SharedPtr<Context> ctx,
    common::SharedPtr<ReqCtx> req,
    RequestCb<Context> callback
    )
{
    HATN_CTX_SCOPE("apiclientexec")

    doExec(std::move(ctx),std::move(req),std::move(callback));
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename MessgaeBufT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,MessgaeBufT,RequestUnitT>::doExec(
        common::SharedPtr<Context> ctx,
        common::SharedPtr<ReqCtx> req,
        RequestCb<Context> callback,
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
        ec=apiError(ApiLibError::QUEUE_OVERFLOW);
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
    if (!req->session()->valid())
    {
        pushToSessionWaitingQueue(std::move(req));
        return;
    }

    // regenerate requiest ID if needed
    if (regenId)
    {
        req.regenId();
    }

    // push request to queue
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

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename MessgaeBufT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,MessgaeBufT,RequestUnitT>::dequeue(Priority priority)
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
        if (front->cancelled())
        {
            queue.pop();
            continue;
        }

        if (!m_connectionPool.canSend(priority))
        {
            break;
        }

        auto req=queue.pop();
        req->taskCtx->onAsyncHandlerEnter();
        sendRequest(std::move(req));
        req->taskCtx->onAsyncHandlerExit();
    }
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename MessgaeBufT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,MessgaeBufT,RequestUnitT>::sendRequest(common::SharedPtr<ReqCtx> req)
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
    m_connectionPool->send(
        reqPtr->taskCtx,
        reqPtr->priority,
        reqPtr->spanBuiffers(),
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

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename MessgaeBufT, typename RequestUnitT>
template <typename Connection>
void Client<RouterTraits,SessionTraits,ContextT,MessgaeBufT,RequestUnitT>::recvResponse(common::SharedPtr<ReqCtx> req, Connection connection)
{
    HATN_CTX_SCOPE("apiclientrecv")

    auto clientCtx=this->sharedMainCtx();
    auto reqPtr=req.get();

    m_connectionPool->recv(        
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
                    auto resp=req->parseResponse();
                    if (resp)
                    {
                        // parsing error
                        req->callback(req->taskCtx,resp.error(),{});
                    }
                    else
                    {
                        auto respWrapper=Response{resp.takeValue(),req->responseData.sharedBuf()};
                        if (resp->status()==static_cast<int>(ResponseStatus::AuthError) && !m_closed)
                        {
                            // process auth error in session
                            refreshSession(std::move(req),std::move(respWrapper));
                        }
                        else
                        {
                            // request is complete
                            req->callback(req->taskCtx,resp.error(),std::move(respWrapper));
                        }
                    }
                }
            }

            // dequeue next request
            m_thread->execAsync(
                [clientCtx{std::move(clientCtx)},this]()
                {
                    std::ignore=clientCtx;
                    dequeue();
                }
            );
        }
    );
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename MessgaeBufT, typename RequestUnitT>
Error Client<RouterTraits,SessionTraits,ContextT,MessgaeBufT,RequestUnitT>::cancel(common::SharedPtr<ReqCtx>& req)
{
    return req->cancel();
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename MessgaeBufT, typename RequestUnitT>
template <typename ContextT1, typename CallbackT>
void Client<RouterTraits,SessionTraits,ContextT,MessgaeBufT,RequestUnitT>::close(
        common::SharedPtr<ContextT1> ctx,
        CallbackT callback,
        bool callbackRequests
    )
{
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

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename MessgaeBufT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,MessgaeBufT,RequestUnitT>::refreshSession(common::SharedPtr<ReqCtx> req, Response resp)
{
    HATN_CTX_SCOPE("apiclientrefreshsession")

    // set session invalid
    req->session()->setInvalid(true);

    // increment counter
    auto& count=m_sessionWaitingReqCount[req->priority()];
    count++;

    // find or create queue
    auto it=m_sessionWaitingQueues[req->session()->id()];
    if (it==m_sessionWaitingQueues.end())
    {
        auto empl=m_sessionWaitingQueues.emplace(req->session()->id(),m_allocatorFactory->defaultDataMemoryResource());
        it=empl.first;
    }

    // enqueue
    auto reqPtr=req.get();
    auto& queue=it->second;
    queue.push_back(std::move(req));

    // invoke session refresh
    auto clientCtx=this->sharedMainCtx();
    reqPtr->session()->refresh(
        clientCtx->id(),
        [clientCtx{std::move(clientCtx)},this](Error ec, const Session<SessionTraits>* session)
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
                    auto it=m_sessionWaitingQueues[sessionId];
                    if (it==m_sessionWaitingQueues.end())
                    {
                       return;
                    }
                    auto& queue=it->second;

                    // handle each waiting request
                    for (auto&& req : queue)
                    {
                        // decrement count
                        auto& count=m_sessionWaitingReqCount[req->priority()];
                        if (count>0)
                        {
                           count--;
                        }

                        // invoke callback on request

                        auto reqPtr=req.get();
                        reqPtr->taskCtx->onAsyncHandlerEnter();

                        HATN_CTX_SCOPE("apiclientrefreshsession")

                        if (ec)
                        {
                           reqPtr->callback(reqPtr->taskCtx,ec,{});
                        }
                        else
                        {
                           doExec(reqPtr->taskCtx,std::move(req),reqPtr->callback,true);
                        }

                        reqPtr->taskCtx->onAsyncHandlerExit();
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

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENT_IPP
