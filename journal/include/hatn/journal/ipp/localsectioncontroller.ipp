/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file section/ipp/localsectioncontroller.ipp
  */

/****************************************************************************/

#ifndef HATNLOCALSECTIONCONTROLLER_IPP
#define HATNLOCALSECTIONCONTROLLER_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/db/dberror.h>

#include <hatn/section/sectionerror.h>
#include <hatn/section/sectiondbmodels.h>
#include <hatn/section/localsectioncontroller.h>

HATN_SECTION_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits>
class LocalSectionController_p
{
    public:

        using Context=typename ContextTraits::Context;

        LocalSectionController_p(
                LocalSectionControllerImpl<ContextTraits>* ctrl,
                std::shared_ptr<db::ModelsWrapper> wrp
            ) : ctrl(ctrl)
        {
            dbModelsWrapper=std::dynamic_pointer_cast<SectionDbModels>(std::move(wrp));
            Assert(dbModelsWrapper,"Invalid SECTION database models dbModelsWrapper, must be section::SectionDbModels");
        }

        LocalSectionControllerImpl<ContextTraits>* ctrl;
        std::shared_ptr<SectionDbModels> dbModelsWrapper;

};

//--------------------------------------------------------------------------

template <typename ContextTraits>
LocalSectionControllerImpl<ContextTraits>::LocalSectionControllerImpl(
        std::shared_ptr<db::ModelsWrapper> modelsWrapper
    ) : d(std::make_shared<LocalSectionController_p<ContextTraits>>(this,std::move(modelsWrapper)))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits>
LocalSectionControllerImpl<ContextTraits>::LocalSectionControllerImpl()
    : d(std::make_shared<LocalSectionController_p<ContextTraits>>(std::make_shared<SectionDbModels>()))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits>
LocalSectionControllerImpl<ContextTraits>::~LocalSectionControllerImpl()
{}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalSectionControllerImpl<ContextTraits>::addTopic(
        common::SharedPtr<Context> ctx,
        CallbackOid callback,
        common::SharedPtr<topic_descriptor::managed> role,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::create(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->topicModel(),
        std::move(role),
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalSectionControllerImpl<ContextTraits>::readTopic(
        common::SharedPtr<Context> ctx,
        CallbackObj<topic_descriptor::managed> callback,
        const du::ObjectId& id,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::read(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->topicModel(),
        id,
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalSectionControllerImpl<ContextTraits>::removeTopic(
        common::SharedPtr<Context> ctx,
        CallbackEc callback,
        const du::ObjectId& id,
        bool /*recursively*/,
        db::Topic topic
    )
{
    //! @todo Remove topic from database

    db::AsyncModelController<ContextTraits>::remove(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->topicModel(),
        id,
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
template <typename QueryBuilderWrapperT>
void LocalSectionControllerImpl<ContextTraits>::listTopics(
        common::SharedPtr<Context> ctx,
        CallbackList callback,
        QueryBuilderWrapperT query,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::list(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->topicModel(),
        std::move(query),
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalSectionControllerImpl<ContextTraits>::updateTopic(
        common::SharedPtr<Context> ctx,
        CallbackEc callback,
        const du::ObjectId& id,
        common::SharedPtr<db::update::Request> request,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::update(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->topicModel(),
        id,
        std::move(request),
        topic
    );
}

//--------------------------------------------------------------------------

HATN_SECTION_NAMESPACE_END

#endif // HATNLOCALSECTIONCONTROLLER_IPP
