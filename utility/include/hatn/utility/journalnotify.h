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

#include <hatn/db/object.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/systemsection.h>
#include <hatn/utility/journal.h>
#include <hatn/utility/notifier.h>

HATN_UTILITY_NAMESPACE_BEGIN

//! @todo Check journal if logging of some acess types is enabled/disabled

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
            lib::string_view objectModel,
            common::pmr::vector<Parameter> params={}
        )
        {
            auto logJournal=[journal=m_journal,
                             callback=std::move(callback),
                             status=std::move(status),
                             op,
                             objectId,
                             objectTopic=TopicType{objectTopic},
                             objectModel,
                             params=std::move(params)
                            ] (auto&& notify, auto&& ctx) mutable
            {
                auto cb=[notify=std::move(notify),callback=std::move(callback),status=std::move(status)](auto&& ctx)
                {
                    notify(std::move(ctx),std::move(callback),std::move(status));
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
                         objectModel
                        ] (auto&& ctx, CallbackT callback, Error status) mutable
            {
                notifier->notify(
                    std::move(ctx),
                    std::move(callback),
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

        template <typename CallbackT>
        void journalOnly(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            Error status,
            const Operation* op,
            const du::ObjectId& objectId,
            const db::Topic& objectTopic,
            lib::string_view objectModel,
            common::pmr::vector<Parameter> params={}
            )
        {
            m_journal->log(
                std::move(ctx),
                std::move(callback),
                status,
                op,
                objectId,
                objectTopic,
                objectModel,
                params
            );
        }

        template <typename CallbackT>
        void journalOnlyList(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            Error status,
            const Operation* op,
            const common::pmr::vector<std::reference_wrapper<const du::ObjectId>> objectIds,
            const db::Topic& objectTopic,
            lib::string_view objectModel,
            common::pmr::vector<Parameter> params={}
            )
        {
            for (auto&& oid: objectIds)
            {
                m_journal->log(
                    std::move(ctx),
                    std::move(callback),
                    status,
                    op,
                    oid.get(),
                    objectTopic,
                    objectModel,
                    params
                );
            }
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
            lib::string_view /*objectModel*/,
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
            lib::string_view /*objectModel*/,
            common::pmr::vector<Parameter> /*params*/={}
            )
        {
            callback(std::move(ctx));
        }

        template <typename ContextT, typename CallbackT>
        void journalOnlyList(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            Error /*status*/,
            const Operation* /*op*/,
            const common::pmr::vector<std::reference_wrapper<const du::ObjectId>> /*objectIds*/,
            const db::Topic& /*objectTopic*/,
            lib::string_view /*objectModel*/,
            common::pmr::vector<Parameter> /*params*/={}
            )
        {
            callback(std::move(ctx));
        }
};

template <typename ImplT, typename ContextTraits>
struct journalNotifyT
{
    template <typename ContextT, typename CallbackT>
    void operator () (
        common::SharedPtr<ContextT> ctx,
        CallbackT callback,
        const du::ObjectId& oid,
        common::pmr::vector<Parameter> params={}
        )
    {
        ContextTraits::contextJournalNotify(ctx).invoke(std::move(ctx),
                                                        std::move(callback),
                                                        Error{},
                                                        operation,
                                                        oid,
                                                        topic,
                                                        model,
                                                        std::move(params)
                                                        );
    };

    std::shared_ptr<ImplT> d;
    const Operation* operation;
    TopicType topic;
    lib::string_view model;
};

template <typename ImplT>
auto journalNotify(std::shared_ptr<ImplT> d, const Operation* operation,lib::string_view topic, const std::string& model)
{
    return journalNotifyT<ImplT,typename ImplT::ContextTraits>{std::move(d),operation,topic,model};
}

template <typename ImplT, typename ContextTraits>
struct journalOnlyT
{
    template <typename ContextT, typename CallbackT>
    void operator () (
        common::SharedPtr<ContextT> ctx,
        CallbackT callback,
        const du::ObjectId& oid,
        common::pmr::vector<Parameter> params={}
        )
    {
        ContextTraits::contextJournalNotify(ctx).journalOnly(std::move(ctx),
                                                        std::move(callback),
                                                        Error{},
                                                        operation,
                                                        oid,
                                                        topic,
                                                        model,
                                                        std::move(params)
                                                        );
    };

    template <typename ContextT, typename CallbackT>
    void operator () (
        common::SharedPtr<ContextT> ctx,
        CallbackT callback,
        const common::pmr::vector<std::reference_wrapper<const du::ObjectId>> objectIds,
        common::pmr::vector<Parameter> params={}
        )
    {
        ContextTraits::contextJournalNotify(ctx).journalOnlyList(std::move(ctx),
                                                             std::move(callback),
                                                             Error{},
                                                             operation,
                                                             objectIds,
                                                             topic,
                                                             model,
                                                             std::move(params)
                                                             );
    };

    std::shared_ptr<ImplT> d;
    const Operation* operation;
    TopicType topic;
    lib::string_view model;
};

template <typename ImplT>
auto journalOnly(std::shared_ptr<ImplT> d, const Operation* operation, lib::string_view topic, lib::string_view model)
{
    return journalOnlyT<ImplT,typename ImplT::ContextTraits>{std::move(d),operation,topic,model};
}

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYJOURNALNOTIFY_H
