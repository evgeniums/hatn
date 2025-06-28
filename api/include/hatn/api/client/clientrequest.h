/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/clientrequest.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTREQUEST_H
#define HATNAPICLIENTREQUEST_H

#include <hatn/common/asiotimer.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/unitwrapper.h>
#include <hatn/dataunit/wirebufchained.h>

#include <hatn/api/api.h>
#include <hatn/api/priority.h>
#include <hatn/api/service.h>
#include <hatn/api/method.h>
#include <hatn/api/client/methodauth.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/responseunit.h>
#include <hatn/api/message.h>
#include <hatn/api/tenancy.h>
#include <hatn/api/client/session.h>
#include <hatn/api/client/clientresponse.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

template <typename ContextT>
using RequestCb=std::function<void (common::SharedPtr<ContextT> ctx, const Error& ec, Response response)>;

template <typename SessionWrapper, typename MessageBufT=du::WireData, typename RequestUnitT=RequestManaged>
struct Request
{
    public:

        using MessageType=Message<MessageBufT>;

        Request(
                const common::pmr::AllocatorFactory* factory,
                const Service& service,
                const Method& method,
                SessionWrapper session,
                MessageType message,
                MethodAuth methodAuth={},
                Priority priority=Priority::Normal,
                uint32_t timeoutMs=0
            ) : m_factory(factory),
                m_session(std::move(session)),
                m_message(std::move(message)),
                m_priority(priority),
                m_timeoutMs(timeoutMs),
                m_pending(true),
                m_cancelled(false),
                m_methodAuth(std::move(methodAuth)),
                responseData(factory),
                m_service(&service),
                m_method(&method),
                m_callbackThread(common::ThreadQWithTaskContext::current())
        {
            responseData.setUseInlineBuffers(true);
        }

        void setPriority(Priority priority) noexcept
        {
            m_priority=priority;
        }

        Priority priority() const noexcept
        {
            return m_priority;
        }

        void setTimeout(uint32_t timeoutMs) noexcept
        {
            m_timeoutMs=timeoutMs;
        }

        uint32_t timeoutMs() const noexcept
        {
            return m_timeoutMs;
        }

        bool pending() const noexcept
        {
            return m_pending;
        }

        bool cancel() noexcept
        {
            if (m_pending)
            {
                return false;
            }
            m_cancelled=true;
            return true;
        }

        bool cancelled() const noexcept
        {
            return m_cancelled;
        }

        const Service* service() const noexcept
        {
            return m_service;
        }

        const Method* method() const noexcept
        {
            return m_method;
        }

        common::ThreadQWithTaskContext* callbackThread() const
        {
            return m_callbackThread;
        }

    protected:

        void setNotPending() noexcept
        {
            m_pending=false;
        }

        lib::string_view id() const noexcept;

        const SessionWrapper* session() const
        {
            return &m_session;
        }

        SessionWrapper* session()
        {
            return &m_session;
        }

        const MessageType& message() const
        {
            return m_message;
        }

        const MethodAuth& methodAuth() const
        {
            return m_methodAuth;
        }

        common::Result<common::SharedPtr<ResponseManaged>> parseResponse() const;

        const common::pmr::AllocatorFactory* m_factory;

        SessionWrapper m_session;
        MessageType m_message;

        Error serialize(
            lib::string_view topic,
            const Tenancy& tenancy
        );
        Error serialize();
        void regenId();
        common::SharedPtr<RequestUnitT> m_unit;

        Priority m_priority;
        uint32_t m_timeoutMs;

        bool m_pending;
        bool m_cancelled;

        MethodAuth m_methodAuth;        
        du::WireBufSolidShared requestData;
        mutable du::WireBufSolidShared responseData;

        const Service* m_service;
        const Method* m_method;

        common::ThreadQWithTaskContext* m_callbackThread;

        template <typename RouterT1, typename SessionWrapper1, typename TaskContextT1, typename MessageBufT1, typename RequestUnitT1>
        friend class Client;
};

template <typename RequestT, typename TaskContextT>
class RequestContext : public RequestT,
                       public common::EnableSharedFromThis<RequestContext<RequestT,TaskContextT>>
{
    public:

        template <typename ...Args>
        RequestContext(
                common::Thread* thread,
                const common::pmr::AllocatorFactory* factory,
                Args&& ...args
            ) : RequestT(factory,std::forward<Args>(args)...),
                timer(thread)
        {
            timer.setAutoAsyncGuardEnabled(false);
        }

    private:

        void setTaskContext(
                common::SharedPtr<TaskContextT> ctx
            ) noexcept
        {
            taskCtx=std::move(ctx);
        }

        void setCallback(
                RequestCb<TaskContextT> cb
            ) noexcept
        {
            callback=std::move(cb);
        }

        void exec()
        {
            if (this->timeoutMs()!=0)
            {
                timer.setPeriodUs(this->timeoutMs*1000);
                auto self=this->sharedFromThis();
                timer.setHandler(
                    [self](common::TimerTypes::Status status)
                    {
                        self->timerHandler(status);
                    }
                );
                timer.start();
            }
        }

        void stopTimer()
        {
            if (timer.isRunning())
            {
                timer.cancel();
            }
        }

        void timerHandler(common::TimerTypes::Status status)
        {
            if (status==common::TimerTypes::Status::Cancel)
            {
                return;
            }

            this->cancel();

            taskCtx->onAsyncHandlerEnter();

            HATN_CTX_SCOPE("apirequesttimeout")

            callback(taskCtx,common::CommonError::TIMEOUT,{});

            taskCtx->onAsyncHandlerExit();
        }

        common::SpanBuffers spanBuffers() const
        {
            common::SpanBuffers bufs{this->m_factory->template dataAllocator<common::SpanBuffer>()};
            auto messageBufs=this->message().chainBuffers(this->m_factory);

            size_t extraCount=1;
            common::ByteArrayShared authHeader;
            if (this->session())
            {
                authHeader=this->session()->authHeader();
                if (authHeader)
                {
                    extraCount++;
                }
            }
            common::ByteArrayShared methodAuthHeader;
            if (this->methodAuth())
            {
                methodAuthHeader=this->methodAuth().authHeader();
                if (methodAuthHeader)
                {
                    extraCount++;
                }
            }
            bufs.reserve(extraCount+messageBufs.size());

            if (authHeader)
            {
                bufs.emplace_back(std::move(authHeader));
            }
            if (methodAuthHeader)
            {
                bufs.emplace_back(std::move(methodAuthHeader));
            }
#if 0
            bufs.emplace_back(this->requestData.sharedMainContainer());
            bufs.insert(bufs.end(), std::make_move_iterator(messageBufs.begin()), std::make_move_iterator(messageBufs.end()));
#else
            bufs.insert(bufs.end(), std::make_move_iterator(messageBufs.begin()), std::make_move_iterator(messageBufs.end()));
            bufs.emplace_back(this->requestData.sharedMainContainer());
#endif
            return bufs;
        }

        common::SharedPtr<TaskContextT> taskCtx;
        RequestCb<TaskContextT> callback;
        common::AsioDeadlineTimer timer;

        template <typename RouterT1, typename SessionWrapper1, typename TaskContextT1, typename MessageBufT1, typename RequestUnitT1>
        friend class Client;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTREQUEST_H
