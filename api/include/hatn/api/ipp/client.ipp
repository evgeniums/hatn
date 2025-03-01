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
#include <hatn/api/client/request.h>
#include <hatn/api/client/client.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
template <typename UnitT>
common::Result<
        common::SharedPtr<typename Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::ReqCtx>
    >
    Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::prepare(
        common::SharedPtr<Session<SessionTraits>> session,
        const Service& service,
        const Method& method,
        const UnitT& content
    )
{
    HATN_CTX_SCOPE("apiclientprepare")

    auto req=common::allocateShared<ReqCtx>(m_allocatorFactory->objectAllocator<ReqCtx>(m_thread,std::move(session)));
    auto ec=req->makeUnit(service,method,content);
    HATN_CTX_CHECK_EC(ec)
    return req;
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
template <typename UnitT>
Error Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::exec(
    common::SharedPtr<Context> ctx,
    common::SharedPtr<Session<SessionTraits>> session,
    const Service& service,
    const Method& method,
    const UnitT& content,
    RequestCb<Context> callback,
    lib::string_view  topic,
    Priority priority,
    uint32_t timeoutMs
    )
{
    HATN_CTX_SCOPE("apiclientexec")

    auto req=common::allocateShared<ReqCtx>(m_allocatorFactory->objectAllocator<ReqCtx>(m_thread,std::move(session),priority,timeoutMs));
    auto ec=req->makeUnit(service,method,content,topic);
    HATN_CTX_CHECK_EC(ec)
    doExec(std::move(ctx),std::move(req),std::move(callback));
    return OK;
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::exec(
    common::SharedPtr<Context> ctx,
    common::SharedPtr<ReqCtx> req,
    RequestCb<Context> callback
    )
{
    HATN_CTX_SCOPE("apiclientexec")

    doExec(std::move(ctx),std::move(req),std::move(callback));
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::doExec(
        common::SharedPtr<Context> ctx,
        common::SharedPtr<ReqCtx> req,
        RequestCb<Context> callback,
        bool regenId
    )
{
    auto it=m_queues.find(req.priority());
    Assert(it!=m_queues.end(),"Unsupported API request priority");
    auto& queue=it->second;

    Error ec;

    if (m_closed)
    {
        ec=commonError(common::CommonError::ABORTED);
    }
    else if (req->priority()!=Priority::Highest && queue.size()>(m_cfg->config().fieldValue(config::max_queue_depth)+m_sessionWaitingReqCount[req->priority()]))
    {
        ec=apiError(ApiLibError::QUEUE_OVERFLOW);
    }

    if (ec)
    {
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

    if (!req->taskCtx)
    {
        req->setTaskContext(std::move(ctx));
        req->setCallback(std::move(callback));
    }

    if (!req->session()->valid())
    {
        pushToSessionWaitingQueue(std::move(req));
        return;
    }

    if (regenId)
    {
        req.regenId();
    }

    if (!queue.busy && queue.empty())
    {
        sendRequest(std::move(req));
    }
    else
    {
        queue.push(std::move(req));
    }
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::dequeue(Queue& queue)
{
    if (m_closed)
    {
        return;
    }

    HATN_CTX_SCOPE("apiclientdequeue")

    while (!queue.empty())
    {
        auto req=queue.pop();
        if (!req->cancelled())
        {
            req->taskCtx->onAsyncHandlerEnter();
            sendRequest(std::move(req));
            req->taskCtx->onAsyncHandlerExit();
            break;
        }
    }
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::sendRequest(common::SharedPtr<ReqCtx> req)
{
    HATN_CTX_SCOPE("apiclientsend")

    auto clientCtx=this->mainCtx().sharedFromThis();

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

                dequeue();
            }
        );
        return;
    }

    // send request
    auto reqPtr=req.get();
    m_connectionPool->send(
        reqPtr->taskCtx,
        reqPtr->spanBuiffers(),
        [req{std::move(req)},clientCtx{std::move(clientCtx)},this](const Error& ec, Connection* connection)
        {
            HATN_CTX_SCOPE("apiclientsendcb")

            if (ec)
            {
                // failed to send, maybe connection is broken
                if (!req->cancelled())
                {
                    req->callback(req->taskCtx,ec,{});
                }

                // nothing todo if client is closed
                if (m_closed)
                {
                    return;
                }

                // dequeue next request
                m_thread->execAsync(
                    [clientCtx{std::move(clientCtx)},this]()
                    {
                        std::ignore=clientCtx;
                        dequeue();
                    }
                );

                return;
            }

            // receive response
            recvResponse(std::move(req),connection);
        },
        reqPtr->priority
    );
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::recvResponse(common::SharedPtr<ReqCtx> req, Connection* connection)
{
    HATN_CTX_SCOPE("apiclientrecv")

    auto clientCtx=this->mainCtx().sharedFromThis();
    auto reqPtr=req.get();

    m_connectionPool->recv(
        connection,
        reqPtr->taskCtx,
        reqPtr->responseData,
        [req{std::move(req)},clientCtx{std::move(clientCtx)},this](const Error& ec)
        {
            HATN_CTX_SCOPE("apiclientrecvcb")

            if (ec)
            {
                // failed to receive, maybe connection is broken
                if (!req->cancelled())
                {
                    req->callback(req->taskCtx,ec,{});
                }
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
                    if (resp->status()==static_cast<int>(ResponseStatus::AuthError))
                    {
                        // process auth error in session
                        refreshSession(std::move(req),resp.value());
                    }
                    else
                    {
                        // request is complete
                        req->callback(req->taskCtx,resp.error(),resp.value());
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

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
Error Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::cancel(common::SharedPtr<ReqCtx>& req)
{
    return req->cancel();
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
template <typename ContextT1, typename CallbackT>
void Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::close(
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

            //! @todo Clear session queues
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
    m_connectionPool.close(std::move(ctx),std::move(callback));
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::refreshSession(common::SharedPtr<ReqCtx> req, const response::type& resp)
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
    auto clientCtx=this->mainCtx().sharedFromThis();
    reqPtr->session()->refresh(resp,
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
        }
    );
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENT_IPP
