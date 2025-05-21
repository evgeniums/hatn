/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file section/localsectioncontroller.h
  */

/****************************************************************************/

#ifndef HATNLOCALSECTIONCONTROLLER_H
#define HATNLOCALSECTIONCONTROLLER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/section/section.h>
#include <hatn/section/sectioncontroller.h>

HATN_SECTION_NAMESPACE_BEGIN

    /**

@todo Add notifier.

Notifier should be used for
1. operations jounral
2. invalidation of caches

 */

template <typename ContextTraits>
class LocalSectionController_p;

template <typename ContextTraits>
class LocalSectionControllerImpl
{
    public:

        using Context=typename ContextTraits::Context;
        using CallbackEc=db::AsyncCallbackEc<Context>;
        using CallbackList=db::AsyncCallbackList<Context>;
        using CallbackOid=db::AsyncCallbackOid<Context>;
        template <typename ModelT>
        using CallbackObj=db::AsyncCallbackObj<Context,ModelT>;

        LocalSectionControllerImpl(
            std::shared_ptr<db::ModelsWrapper> modelsWrapper
        );

        LocalSectionControllerImpl();

        ~LocalSectionControllerImpl();
        LocalSectionControllerImpl(const LocalSectionControllerImpl&)=delete;
        LocalSectionControllerImpl(LocalSectionControllerImpl&&)=default;
        LocalSectionControllerImpl& operator=(const LocalSectionControllerImpl&)=delete;
        LocalSectionControllerImpl& operator=(LocalSectionControllerImpl&&)=default;

        void addTopic(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<topic_descriptor::managed> section,
            db::Topic topic=SystemTopic
        );

        void readTopic(
            common::SharedPtr<Context> ctx,
            CallbackObj<topic_descriptor::managed> callback,
            const du::ObjectId& id,
            db::Topic topic=SystemTopic
        );

        void removeTopic(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            bool recursively,
            db::Topic topic=SystemTopic
        );

        template <typename QueryBuilderWrapperT>
        void listTopics(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            QueryBuilderWrapperT query,
            db::Topic topic=SystemTopic
        );

        void updateTopic(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            common::SharedPtr<db::update::Request> request,
            db::Topic topic=SystemTopic
        );

    private:

        std::shared_ptr<LocalSectionController_p<ContextTraits>> d;

        template <typename ContextTraits1>
        friend class LocalSectionController_p;
};

template <typename ContextTraits>
using LocalSectionController=SectionController<typename ContextTraits::Context,LocalSectionControllerImpl<ContextTraits>>;

HATN_SECTION_NAMESPACE_END

#endif // HATNLOCALSECTIONCONTROLLER_H
