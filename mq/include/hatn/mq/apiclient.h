/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/apiclient.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNBMQAPICLIENT_H
#define HATNBMQAPICLIENT_H

#include <hatn/common/taskcontext.h>

#include <hatn/api/message.h>
#include <hatn/api/client/serviceclient.h>

#include <hatn/mq/mq.h>

HATN_MQ_NAMESPACE_BEGIN

//! @todo Move it to mq service definitions
constexpr const char* ProducerPostMethod="producer_post";
class MethodProducerPost : public api::Method
{
    public:

    MethodProducerPost() : api::Method(ProducerPostMethod)
    {}
};

template <typename Traits>
class ApiClient : public common::pmr::WithFactory,
                  public common::TaskSubcontext
{
    public:

        using MappedServiceClients=typename Traits::MappedServiceClients;
        using MappedServiceClientsCtx=typename Traits::MappedServiceClientsCtx;
        using Message=typename Traits::Message;

        ApiClient(
                common::SharedPtr<MappedServiceClientsCtx> serviceClients,
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : common::pmr::WithFactory(factory),
                m_serviceClients(std::move(serviceClients))
        {}

        void setServiceClients(common::SharedPtr<MappedServiceClientsCtx> serviceClients)
        {
            m_serviceClients=std::move(serviceClients);
        }

        common::SharedPtr<MappedServiceClientsCtx> serviceClientsShared() const
        {
            return m_serviceClients;
        }

        const MappedServiceClientsCtx* serviceClients() const
        {
            if (!m_serviceClients)
            {
                return nullptr;
            }

            auto& subctx=m_serviceClients->template get<MappedServiceClients>();
            return &subctx;
        }

        template <typename ContextT, typename CallbackT, typename MessageT>
        void producerPost(
                common::SharedPtr<ContextT> ctx,
                CallbackT callback,
                lib::string_view topic,
                common::SharedPtr<MessageT> msg
            )
        {
            serviceClients()->findServiceClient(
                std::move(ctx),
                [selfCtx{this->sharedMainCtx()},this,callback{std::move(callback)},msg{std::move(msg)},topic](auto ctx, auto clientCtx, auto& client)
                {
                    Message reqMsg;
                    auto ec=reqMsg.setContent(*msg,this->factory());
                    if (ec)
                    {
                        //! @todo log error
                        callback(std::move(ctx),ec,std::move(msg));
                        return;
                    }

                    auto execCb=[clientCtx{std::move(clientCtx)},ctx{std::move(ctx)},msg{std::move(msg)}](auto, const Error& ec, api::client::Response response)
                    {
                        if (ec)
                        {
                            //! @todo log error
                            callback(std::move(ctx),ec,std::move(msg));
                            return;
                        }

                        // for post method good response is not needed to be parsed
                        std::ignore=response;
                        callback(std::move(ctx),ec,std::move(msg));
                    };
                    client.exec(ctx,std::move(execCb),m_methodProducerPost,std::move(reqMsg),topic);
                },
                topic
            );
        }

    private:

        common::SharedPtr<MappedServiceClientsCtx> m_serviceClients;

        MethodProducerPost m_methodProducerPost;
};

HATN_MQ_NAMESPACE_END

#endif // HATNBMQAPICLIENT_H
