/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/request.h
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
#include <hatn/api/client/session.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

template <typename ContextT>
using RequestCb=std::function<void (common::SharedPtr<ContextT> ctx, const Error& ec, Response response)>;

template <typename SessionTraits, typename MessageBufT=du::WireData, typename RequestUnitT=request::shared_managed>
struct Request
{
    public:

        using MessageType=Message<MessageBufT>;

        Request(
                const common::pmr::AllocatorFactory* factory,
                common::SharedPtr<Session<SessionTraits>> session,
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
                m_methodAuth(std::move(methodAuth))
        {
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

    protected:

        bool setNotPending() noexcept
        {
            m_pending=false;
        }

        lib::string_view id() const noexcept;

        common::Result<common::SharedPtr<ResponseManaged>> parseResponse() const;

        common::SharedPtr<Session<SessionTraits>>& session()
        {
            return m_session;
        }

        const MessageType& message() const
        {
            return m_message;
        }

        const MethodAuth& methodAuth() const
        {
            return m_methodAuth;
        }

        const common::pmr::AllocatorFactory* m_factory;

        common::SharedPtr<Session<SessionTraits>> m_session;
        MessageType m_message;

        Error serialize(
            const Service& service,
            const Method& method,
            lib::string_view topic={}
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

        template <typename ...T>
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
                timer(thread),                
                responseData(factory)
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
            auto messageBufs=this->message()->buffers();

            size_t extraCount=1;
            common::ByteArrayShared authHeader;
            if (this->sesion())
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
                methodAuthHeader=this->methodAuth()->authHeader();
                if (authHeader)
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

            bufs.emplace_back(this->requestData.sharedMainContainer());
            bufs.insert(bufs.end(), std::make_move_iterator(messageBufs.begin()), std::make_move_iterator(messageBufs.end()));
            return bufs;
        }        

        common::SharedPtr<TaskContextT> taskCtx;
        RequestCb<TaskContextT> callback;
        common::AsioDeadlineTimer timer;

        du::WireBufSolidShared responseData;

        template <typename ...T>
        friend class Client;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTREQUEST_H
