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

#include <hatn/logcontext/postasync.h>

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

    auto req=common::allocateShared<ReqCtx>(m_allocatorFactory->objectAllocator<ReqCtx>(),m_thread,m_allocatorFactory,service,method,std::move(session),std::move(message),std::move(methodAuth));
    const Tenancy& tenancy=Tenancy::contextTenancy(*ctx);
    auto ec=req->serialize(topic,tenancy);
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

    auto req=common::allocateShared<ReqCtx>(m_allocatorFactory->objectAllocator<ReqCtx>(),m_thread,m_allocatorFactory,service,method,std::move(session),std::move(message),std::move(methodAuth),priority,timeoutMs);
    const Tenancy& tenancy=Tenancy::contextTenancy(*ctx);
    auto ec=req->serialize(topic,tenancy);
    HATN_CTX_CHECK_EC(ec)
    doExec(std::move(ctx),makeAsyncCallback(std::move(callback)),std::move(req));
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

    doExec(std::move(ctx),makeAsyncCallback(std::move(callback)),std::move(req));
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::doExec(
        common::SharedPtr<Context> ctx,
        RequestCbInternal<Context> callback,
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
    else if (req->priority()!=Priority::Highest && queue.size()>(config().fieldValue(config::max_queue_depth)+m_sessionWaitingReqCount[req->priority()]))
    {
        // check if queue is filled
        ec=apiLibError(ApiLibError::QUEUE_OVERFLOW);
    }

    if (ec)
    {
        callback("apiclientexec",std::move(ctx),std::move(ec),Response{});
        return;
    }

    // set task context and callback
    if (!req->taskCtx)
    {
        req->setTaskContext(std::move(ctx));
        req->setCallback(std::move(callback));
    }

    // if session not ready then push to waiting queue for this session
    if (!req->session().isNull() && !req->session().isValid())
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
    auto priority=req->priority();
    queue.push(std::move(req));

    // dequeue requests
    postDequeue(priority);
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::postDequeue(Priority priority)
{
    common::postAsyncTask(
        m_thread,
        sharedMainCtx(),
        [priority,this](auto)
        {
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

    //! @todo maybe implement weighted priorities handling
    auto handlePriorityQueue=[this,priority](Priority p)
    {
        std::ignore=priority;

        auto it=m_queues.find(p);
        auto& queue=it->second;

        while (!queue.empty())
        {
            auto* front=queue.front();
            if ((*front)->cancelled())
            {
                queue.pop();
                continue;
            }

            if (!m_connectionPool.canSend(p))
            {
                return false;
            }

            auto req=queue.pop();
            auto taskCtx=req->taskCtx;
            taskCtx->onAsyncHandlerEnter();
            sendRequest(std::move(req));
            taskCtx->onAsyncHandlerExit();
        }

        return false;
    };
    handleByPriority(handlePriorityQueue);
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
        // dequeue next request
        postDequeue(req->priority());
        req->callback("apiclientsend",req->taskCtx,std::move(ec),Response{});

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
                    req->callback(req->taskCtx,ec,Response{});
                }

                // nothing to do if client is closed
                if (m_closed)
                {
                    return;
                }

                // dequeue next request
                postDequeue(req->priority());
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
        [req{std::move(req)},clientCtx{std::move(clientCtx)},this](Error ec)
        {
            HATN_CTX_SCOPE("apiclientrecvcb")

            if (!req->cancelled())
            {
                bool invokeCallback=true;
                Response respWrapper{};

                if (!ec)
                {
                    auto r=req->parseResponse();
                    if (r)
                    {
                        // parsing error
                        ec=r.takeError();
                    }
                    else
                    {
                        const auto& resp=r.value();
                        const auto& respField=resp->field(protocol::response::message);
                        auto respMessage=respField.skippedNotParsedContent();
                        respWrapper=Response{r.takeValue(),req->responseData.sharedMainContainer(),std::move(respMessage)};
                        auto status=respWrapper.status();
                        if (!respWrapper.isSuccess())
                        {
                            if (status==protocol::ResponseStatus::AuthError && !req->session().isNull())
                            {
                                // process auth error in session
                                if (!m_closed)
                                {
                                    invokeCallback=false;
                                    // set session invalid
                                    req->session().setValid(false);
                                    // refresh session
                                    refreshSession(std::move(req),std::move(respWrapper));
                                }
                            }

                            if (invokeCallback)
                            {
                                // parse error response
                                ec=respWrapper.parseError(m_allocatorFactory);
                            }
                        }
                    }
                }

                if (invokeCallback)
                {
                    req->callback("apiclientresp",req->taskCtx,ec,std::move(respWrapper));
                }
            }

            // dequeue next request
            postDequeue(req->priority());
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
    m_closed.store(true);

    postAsync(
        "apiclientclose",
        m_thread,
        std::move(ctx),
        [this,callbackRequests,clientCtx=sharedMainCtx()](auto ctx, auto cb)
        {
            auto abortRequest=[](auto req)
            {
                if (!req->cancelled())
                {
                    req->callback("apiclientclose",req->taskCtx,commonError(common::CommonError::ABORTED),{});
                }
            };

            // clear queues
            for (auto&& it: m_queues)
            {
                if (callbackRequests)
                {
                    while (!it.second.empty())
                    {
                        auto req=it.second.pop();
                        abortRequest(std::move(req));
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
                        abortRequest(std::move(req));
                    }
                }
            }
            m_sessionWaitingQueues.clear();

            // close connection pool
            m_connectionPool.close(ctx,
                                   [ctx,clientCtx{std::move(clientCtx)},cb{std::move(cb)}](const auto& ec)
                                   {
                                       std::ignore=clientCtx;
                                       cb(ctx,ec);
                                   }
            );
        },
        std::move(callback)
    );
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::refreshSession(common::SharedPtr<ReqCtx> req, Response resp)
{
    postAsync(
        "apiclientrefreshsession",
        m_thread,
        sharedMainCtx(),
        [req=std::move(req),resp=std::move(resp),this](auto)
        {
            if (m_closed)
            {
                return;
            }

            // if session is valid then exec request
            auto reqPtr=req.get();
            if (reqPtr->session().isValid())
            {
                doExec(reqPtr->taskCtx,reqPtr->callback,std::move(req),true);
                return;
            }

            // increment counter
            auto& count=m_sessionWaitingReqCount[req->priority()];
            count++;

            // find or create queue
            auto it=m_sessionWaitingQueues.find(req->session().id());
            if (it==m_sessionWaitingQueues.end())
            {
                auto empl=m_sessionWaitingQueues.emplace(req->session().id(),m_allocatorFactory->objectMemoryResource());
                it=empl.first;
            }

            // enqueue            
            auto& queue=it->second;
            queue.push(std::move(req));

            // skip if already refreshing
            if (reqPtr->session().isRefreshing())
            {
                return;
            }

            // define refresh callback
            auto refreshCb=[this,sessionId=reqPtr->session().id()](auto, const Error& ec)
            {
                Assert(m_thread->id()==common::Thread::currentThreadID(),"Session must work in the same thread with client");

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

                    auto reqPtr=req.get();
                    {
                        if (ec)
                        {
                            req->callback("apiclientrefreshsession",req->taskCtx,ec,Response{});
                        }
                        else
                        {
                            // enqueue request again
                            doExec(reqPtr->taskCtx,reqPtr->callback,std::move(req),true);
                        }
                    }
                }

                // delete queue for this session
                m_sessionWaitingQueues.erase(it);
            };

            // invoke session refresh
            reqPtr->session().refresh(
                sharedMainCtx(),                
                std::move(refreshCb),
                this,
                std::move(resp)
            );
        }
    );
}

//---------------------------------------------------------------

template <typename RouterT, typename SessionWrapperT, typename ContextT, typename MessageBufT, typename RequestUnitT>
void Client<RouterT,SessionWrapperT,ContextT,MessageBufT,RequestUnitT>::pushToSessionWaitingQueue(common::SharedPtr<ReqCtx> req)
{
    postAsync(
        "apiclientpushreqwaitsession",
        m_thread,
        sharedMainCtx(),
        [req=std::move(req),this](auto)
        {
            if (m_closed)
            {
                return;
            }

            // find session queue
            auto it=m_sessionWaitingQueues.find(req->session().id());
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
    );
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENT_IPP
