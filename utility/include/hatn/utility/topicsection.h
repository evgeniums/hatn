/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/sectioncontroller.h
  */

/****************************************************************************/

#ifndef HATNSECTIONCONTROLLER_H
#define HATNSECTIONCONTROLLER_H

#include <hatn/common/sharedptr.h>

#include <hatn/db/topic.h>
#include <hatn/db/indexquery.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/sectionmodels.h>
#include <hatn/utility/sectiondbmodels.h>
#include <hatn/utility/systemsection.h>

HATN_UTILITY_NAMESPACE_BEGIN

template <typename ContextTraits>
struct topicSectionT
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
        topicSection(std::move(ctx),std::move(callback),dbModelsWrapper,examineTopic,inTopic);
    }

    private:

        template <typename CallbackT>
        static void topicSection(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const SectionDbModels& dbModelsWrapper,
            const du::ObjectId& examineTopicOid,
            db::Topic examineTopic,
            db::Topic inTopic
        )
        {
            TopicType topic{inTopic};
            auto query=db::wrapQueryBuilder(
                [examineTopic=TopicType{examineTopic},topic,examineTopicOid]()
                {
                    // query either by topic name or topic oid
                    if (examineTopicOid.isNull())
                    {
                        return db::makeQuery(topicNameIdx(),
                                             db::where(topic_descriptor::name,db::query::eq,examineTopic),
                                             topic
                                             );
                    }
                    return db::makeQuery(db::oidIdx(),
                                         db::where(db::Oid,db::query::eq,examineTopicOid),
                                         topic
                                         );
                },
                topic
            );

            auto cb=[callback{std::move(callback)},&dbModelsWrapper,topic](auto ctx, auto result) mutable
            {
                if (result)
                {
                    //! @todo Log error?
                    callback(std::move(ctx),result.takeError(),du::ObjectId{});
                    return;
                }

                const auto* descr=result->get();
                if (descr->fieldValue(topic_descriptor::section))
                {
                    callback(std::move(ctx),Error{},result.takeValue());
                    return;
                }

                topicSection(std::move(ctx),std::move(callback),dbModelsWrapper,descr->fieldValue(db::Oid),{},topic);
            };

            auto& db=ContextTraits::ContextDb(ctx);
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
constexpr topicSectionT<ContextTraits> topicSection{};

template <typename ContextTraits>
struct sectionTopicT
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
        auto cb=[callback=std::move(callback)](auto ctx, const Error& ec, common::SharedPtr<topic_descriptor::managed> topicDescriptor)
        {
            if (ec)
            {
                callback(std::move(ctx),ec,db::Topic{});
                return;
            }
            callback(std::move(ctx),Error{},db::Topic{topicDescriptor->fieldValue(db::Oid).string()});
        };
        topicSection<ContextTraits>(std::move(ctx),std::move(cb),dbModelsWrapper,{},examineTopic,inTopic);
    }
};
template <typename ContextTraits>
constexpr sectionTopicT<ContextTraits> sectionTopic{};

HATN_UTILITY_NAMESPACE_END

#endif // HATNSECTIONCONTROLLER_H
