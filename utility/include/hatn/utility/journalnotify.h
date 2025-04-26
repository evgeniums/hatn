/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/journalnotify.h
  */

/****************************************************************************/

#ifndef HATNUTILITYJOURNALNOTIFY_H
#define HATNUTILITYJOURNALNOTIFY_H

#include <hatn/common/meta/chain.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/systemsection.h>
#include <hatn/utility/journal.h>
#include <hatn/utility/notifier.h>

HATN_UTILITY_NAMESPACE_BEGIN

template <typename ContextTraits, typename JournalT, typename NotifierT>
class JournalNotify
{
    public:

        using Context=typename ContextTraits::Context;

        JournalNotify(
                std::shared_ptr<JournalT> journal={},
                std::shared_ptr<NotifierT> notifier={}
            ) : m_journal(std::move(journal)),
                m_notifier(std::move(notifier))
        {}

        template <typename CallbackT>
        void invoke(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            Error status,
            const Operation* op,
            const du::ObjectId& objectId,
            const db::Topic& objectTopic,
            const std::string& objectModel,
            bool journalOnly=false,
            common::pmr::vector<Parameter> params={}
        )
        {
            auto logJournal=[journal=m_journal,
                             journalOnly,
                             callback=std::move(callback),
                             status=std::move(status),
                             op,
                             objectId,
                             objectTopic=TopicType{objectTopic},
                             &objectModel,
                             params=std::move(params)
                            ] (auto&& notify, auto&& ctx) mutable
            {
                auto cb=[journalOnly,notify=std::move(notify),callback=std::move(callback),status=std::move(status)](auto&& ctx)
                {
                    if (journalOnly)
                    {
                        callback(std::move(ctx));
                    }
                    else
                    {
                        notify(std::move(ctx),std::move(callback),std::move(status));
                    }
                };

                journal->log(
                    std::move(ctx),
                    std::move(cb),
                    status,
                    op,
                    objectId,
                    objectTopic,
                    objectModel,
                    params
                );
            };

            auto notify=[notifier=m_notifier,
                         op,
                         objectId,
                         objectTopic=TopicType{objectTopic},
                         &objectModel
                        ] (auto&& ctx, CallbackT callback, Error status) mutable
            {
                auto cb=[callback=std::move(callback)](auto&& ctx)
                {
                    callback(std::move(ctx));
                };

                notifier->notify(
                    std::move(ctx),
                    std::move(cb),
                    status,
                    op,
                    objectId,
                    objectTopic,
                    objectModel
                );
            };

            auto chain=hatn::chain(
                std::move(logJournal),
                std::move(notify)
            );
            chain(std::move(ctx));
        }

        void setJournal(std::shared_ptr<JournalT> journal) noexcept
        {
            m_journal=std::move(journal);
        }

        void setNotifier(std::shared_ptr<NotifierT> notifier) noexcept
        {
            m_notifier=std::move(notifier);
        }

    private:

        std::shared_ptr<JournalT> m_journal;
        std::shared_ptr<NotifierT> m_notifier;
};

class JournalNotifyNone
{
    public:

        template <typename ContextT, typename CallbackT>
        void invoke(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            Error /*status*/,
            const Operation* /*op*/,
            const du::ObjectId& /*objectId*/,
            const db::Topic& /*objectTopic*/,
            const std::string& /*objectModel*/,
            common::pmr::vector<Parameter> /*params*/={}
            )
        {
            callback(std::move(ctx));
        }

        template <typename ContextT, typename CallbackT>
        void jouranlOnly(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            Error /*status*/,
            const Operation* /*op*/,
            const du::ObjectId& /*objectId*/,
            const db::Topic& /*objectTopic*/,
            const std::string& /*objectModel*/,
            common::pmr::vector<Parameter> /*params*/={}
            )
        {
            callback(std::move(ctx));
        }
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYJOURNALNOTIFY_H
