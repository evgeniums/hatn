/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/topicdescriptor.h
  */

/****************************************************************************/

#ifndef HATNTOPICDESCRIPTOR_H
#define HATNTOPICDESCRIPTOR_H

#include <hatn/common/sharedptr.h>

#include <hatn/db/topic.h>
#include <hatn/db/indexquery.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/sectionmodels.h>
#include <hatn/utility/sectiondbmodels.h>
#include <hatn/utility/systemsection.h>

HATN_UTILITY_NAMESPACE_BEGIN

using TopicNameOrOidRef=lib::variant<
        db::Topic,
        std::reference_wrapper<const du::ObjectId>
    >;

struct TopicNameOrOid
{
    TopicNameOrOid(const TopicNameOrOidRef& ref)
    {
        if (ref.index()==0)
        {
            data=TopicType{lib::variantGet<db::Topic>(ref)};
        }
        else
        {
            data=lib::variantGet<std::reference_wrapper<const du::ObjectId>>(ref).get();
        }
    }

    TopicNameOrOid(db::Topic ref)
    {
        auto oid=du::ObjectId::fromString(ref.topic());
        if (oid)
        {
            data=TopicType{ref};
        }
        else
        {
            data=oid.value();
        }
    }

    TopicNameOrOid(const du::ObjectId& oid) : data(oid)
    {}

    lib::variant<
        TopicType,
        du::ObjectId
    > data;
};

template <typename ContextTraits>
struct topicDescriptorT
{
    using Context=typename ContextTraits::Context;

    template <typename CallbackT>
    auto operator ()(
        common::SharedPtr<Context> ctx,
        CallbackT callback,
        const SectionDbModels& dbModelsWrapper,
        const TopicNameOrOidRef& examineTopic,
        db::Topic inTopic=SystemTopic,
        bool findSectionTopic=false
    ) const
    {
        if (examineTopic.index()==0)
        {
            find(std::move(ctx),std::move(callback),dbModelsWrapper,lib::variantGet<db::Topic>(examineTopic),inTopic,findSectionTopic);
        }
        else
        {
            find(std::move(ctx),std::move(callback),dbModelsWrapper,lib::variantGet<std::reference_wrapper<const du::ObjectId>>(examineTopic),inTopic,findSectionTopic);
        }
    }

    private:

        static auto wrapQuery(db::Topic examineTopic, db::Topic topic)
        {
            return
                db::wrapQueryBuilder(
                    [topic=TopicType{topic},examineTopic=TopicType{examineTopic}]()
                    {
                        return db::makeQuery(topicNameIdx(),
                                      db::where(topic_descriptor::name,db::query::eq,topic),
                                      topic
                                      );
                    },
                    topic
                );
        }

        static auto wrapQuery(const du::ObjectId& examineTopic, db::Topic topic)
        {
            return
                db::wrapQueryBuilder(
                    [topic=TopicType{topic},examineTopic]()
                    {
                        return db::makeQuery(db::oidIdx(),
                                             db::where(db::Oid,db::query::eq,examineTopic),
                                             topic
                                             );
                    },
                    topic
                );
        }

        template <typename TopicT, typename CallbackT>
        static void find(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const SectionDbModels& dbModelsWrapper,
            TopicT&& examineTopic,
            db::Topic inTopic,
            bool findSectionTopic
        )
        {
            auto query=wrapQuery(examineTopic,inTopic);

            auto cb=[callback{std::move(callback)},&dbModelsWrapper,findSectionTopic](auto ctx, auto result) mutable
            {
                if (result)
                {
                    //! @todo Log error?
                    callback(std::move(ctx),result.takeError(),{});
                    return;
                }

                const auto* descr=result->get();
                // if (!findSectionTopic || descr->fieldValue(topic_descriptor::section))
                // {
                //     // done
                //     callback(std::move(ctx),Error{},result.takeValue());
                //     return;
                // }

                // iterate next
                find(std::move(ctx),std::move(callback),dbModelsWrapper,descr->fieldValue(db::Oid),descr->fieldValue(topic_descriptor::parent).string(),findSectionTopic);
            };

            auto& db=ContextTraits::contextDb(ctx);
            db.dbClient(inTopic)->findOne(
                std::move(ctx),
                std::move(cb),
                dbModelsWrapper.topicModel(),
                query,
                inTopic
            );
        }
};
template <typename ContextTraits>
constexpr topicDescriptorT<ContextTraits> topicDescriptor{};

template <typename ContextTraits>
struct topicOidT
{
    using Context=typename ContextTraits::Context;

    template <typename CallbackT>
    auto operator ()(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const SectionDbModels& dbModelsWrapper,
            db::Topic examineTopic,
            db::Topic inTopic=SystemTopic,
            bool findSectionTopic=false
        ) const
    {
        auto cb=[callback=std::move(callback)](auto ctx, const Error& ec, common::SharedPtr<topic_descriptor::managed> topicDescriptor)
        {
            if (ec)
            {
                callback(std::move(ctx),ec,db::Topic{});
                return;
            }
            callback(std::move(ctx),Error{},topicDescriptor->fieldValue(db::Oid));
        };
        topicDescriptor<ContextTraits>(std::move(ctx),std::move(cb),dbModelsWrapper,{},examineTopic,inTopic,findSectionTopic);
    }
};
template <typename ContextTraits>
constexpr topicOidT<ContextTraits> topicOid{};

template <typename ContextTraits>
struct sectionTopicOidT
{
    using Context=typename ContextTraits::Context;

    template <typename CallbackT>
    auto operator ()(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const SectionDbModels& dbModelsWrapper,
            db::Topic examineTopic,
            db::Topic inTopic=SystemTopic
        ) const
    {
        topicOid<ContextTraits>(std::move(ctx),std::move(callback),dbModelsWrapper,{},examineTopic,inTopic,true);
    }
};
template <typename ContextTraits>
constexpr sectionTopicOidT<ContextTraits> sectionTopicOid{};

HATN_UTILITY_NAMESPACE_END

#endif // HATNTOPICDESCRIPTOR_H
