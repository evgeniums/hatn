/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file section/sectiondbmodels.h
  */

/****************************************************************************/

#ifndef HATNSECTIONDBMODELS_H
#define HATNSECTIONDBMODELS_H

#include <hatn/db/model.h>
#include <hatn/db/modelswrapper.h>

#include <hatn/utility/sectionmodels.h>
#include <hatn/utility/withmacdb.h>

HATN_UTILITY_NAMESPACE_BEGIN

HATN_DB_INDEX(topicParentIdx,topic_descriptor::parent)
HATN_DB_INDEX(topicNameIdx,topic_descriptor::name)
HATN_DB_INDEX(topicSectionIdx,topic_descriptor::section)
HATN_DB_MODEL_PROTOTYPE(topicModel,topic_descriptor,topicParentIdx(),topicNameIdx(),macPolicyIdx())

class SectionDbModels : public db::ModelsWrapper
{
    public:

        SectionDbModels(std::string prefix={}) : db::ModelsWrapper(std::move(prefix))
        {}

        const auto& topicModel() const
        {
            return db::makeModelFromPrototype(prefix(),HATN_UTILITY_NAMESPACE::topicModel);
        }

        auto models()
        {
            return hana::make_tuple(
                [this](){return topicModel();}
            );
        }
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNSECTIONDBMODELS_H
