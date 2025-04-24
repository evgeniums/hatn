/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/notifier.h
  */

/****************************************************************************/

#ifndef HATNUTILITYNOTIFIER_H
#define HATNUTILITYNOTIFIER_H

#include <hatn/common/objecttraits.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/allocatoronstack.h>
\
#include <hatn/dataunit/objectid.h>

#include <hatn/db/topic.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/operation.h>

HATN_UTILITY_NAMESPACE_BEGIN

template <typename Traits>
class Notifier : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        template <typename ContextT, typename CallbackT>
        void notify(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const Operation* op,
            const du::ObjectId& objectId,
            const db::Topic& objectTopic,
            const char* objectModel
        )
        {
            this->traits(std::move(ctx),std::move(callback),op,objectId,objectTopic,objectModel);
        }
};

class NotifierNone
{
    public:

    template <typename ContextT, typename CallbackT>
    void notify(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const Operation* /*op*/,
            const du::ObjectId& /*objectId*/,
            const db::Topic& /*objectTopic*/,
            const char* /*objectModel*/
        )
        {
            callback(std::move(ctx));
        }
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYNOTIFIER_H
