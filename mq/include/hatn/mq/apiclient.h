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
#include <hatn/mq/message.h>
#include <hatn/mq/methods.h>

HATN_MQ_NAMESPACE_BEGIN

template <typename ClientWithAuthContextT, typename ClientWithAuthT>
struct ApiClientDefaultTraits
{
    using MappedClients=api::client::SingleClientWithAuth<ClientWithAuthContextT,ClientWithAuthT>;
    using Message=message::managed;
};

template <typename Traits>
class ApiClient : public common::pmr::WithFactory,
                  public common::TaskSubcontext
{
    public:

        using MappedClients=typename Traits::MappedClients;
        using Message=typename Traits::Message;

        ApiClient(
                api::Service mqService,
                common::SharedPtr<MappedClients> topicClients={},
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : common::pmr::WithFactory(factory),
                m_mqService(std::move(mqService)),
                m_topicClients(std::move(topicClients))
        {}

        ApiClient(
            lib::string_view serviceName,
            common::SharedPtr<MappedClients> topicClients={},
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault(),
            uint8_t serviceVersion=1
            ) : ApiClient(api::Service{serviceName,serviceVersion},std::move(topicClients),factory)
        {}

        void setTopicClients(common::SharedPtr<MappedClients> topicClients)
        {
            m_topicClients=std::move(topicClients);
        }

        common::SharedPtr<MappedClients> topicClients() const
        {
            return m_topicClients;
        }

        void setMqService(api::Service mqService)
        {
            m_mqService=std::move(mqService);
        }

        const api::Service& mqService() const
        {
            return m_mqService;
        }

        template <typename ContextT, typename CallbackT, typename MessageT>
        void producerPost(
                common::SharedPtr<ContextT> ctx,
                CallbackT callback,
                lib::string_view topic,
                common::SharedPtr<MessageT> msg
            )
        {
            m_topicClients->findClientWithAuth(
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
                    client.exec(ctx,std::move(execCb),m_mqService,m_methodProducerPost,std::move(reqMsg),topic);
                },
                topic
            );
        }

    private:

        api::Service m_mqService;
        common::SharedPtr<MappedClients> m_topicClients;

        MethodProducerPost m_methodProducerPost;
};

HATN_MQ_NAMESPACE_END

#endif // HATNBMQAPICLIENT_H
