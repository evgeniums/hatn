/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file section/sectioncontroller.h
  */

/****************************************************************************/

#ifndef HATNSECTIONCONTROLLER_H
#define HATNSECTIONCONTROLLER_H

#include <hatn/common/objecttraits.h>

#include <hatn/db/update.h>
#include <hatn/db/indexquery.h>
#include <hatn/db/asyncmodelcontroller.h>

#include <hatn/section/section.h>
#include <hatn/section/sectionmodels.h>

HATN_SECTION_NAMESPACE_BEGIN

constexpr const char* SystemTopic="system";

template <typename ContextT, typename Traits>
class SectionController : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        using Context=ContextT;
        using CallbackEc=db::AsyncCallbackEc<Context>;
        using CallbackList=db::AsyncCallbackList<Context>;
        using CallbackOid=db::AsyncCallbackOid<Context>;
        template <typename ModelT>
        using CallbackObj=db::AsyncCallbackObj<Context,ModelT>;

        void addTopic(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<topic_descriptor::managed> section,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().addTopic(std::move(ctx),std::move(callback),std::move(section),topic);
        }

        void readTopic(
            common::SharedPtr<Context> ctx,
            CallbackObj<topic_descriptor::managed> callback,
            const du::ObjectId& id,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().readTopic(std::move(ctx),std::move(callback),id,topic);
        }

        void removeTopic(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            bool recursively,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().removeTopic(std::move(ctx),std::move(callback),id,recursively,topic);
        }

        template <typename QueryBuilderWrapperT>
        void listTopics(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            QueryBuilderWrapperT query,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().listTopics(std::move(ctx),std::move(callback),std::move(query),topic);
        }

        void updateTopic(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            common::SharedPtr<db::update::Request> request,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().updateTopic(std::move(ctx),std::move(callback),id,std::move(request),topic);
        }
};

HATN_SECTION_NAMESPACE_END

#endif // HATNSECTIONCONTROLLER_H
