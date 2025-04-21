/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file db/modelcontroller.h
  */

/****************************************************************************/

#ifndef HATNASYNCMODELCONTROLLER_H
#define HATNASYNCMODELCONTROLLER_H

#include <hatn/db/update.h>
#include <hatn/db/indexquery.h>

HATN_DB_NAMESPACE_BEGIN

template <typename ContextT>
using AsyncCallbackEc=std::function<void (common::SharedPtr<ContextT>, const Error&)>;
template <typename ContextT>
using AsyncCallbackList=std::function<void (common::SharedPtr<ContextT>, Result<common::pmr::vector<DbObject>>)>;
template <typename ContextT>
using AsyncCallbackOid=std::function<void (common::SharedPtr<ContextT>, const Error&, const du::ObjectId&)>;

template <typename ContextTraits>
class AsyncModelController
{
    public:

        using Context=typename ContextTraits::Context;
        using CallbackEc=AsyncCallbackEc<Context>;
        using CallbackList=AsyncCallbackList<Context>;
        using CallbackOid=AsyncCallbackOid<Context>;

        template <typename ModelT, typename UnitT>
        static void create(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            const ModelT& model,
            common::SharedPtr<UnitT> obj,
            Topic topic
        )
        {
            auto unit=obj.get();
            db::initObject(*unit);
            auto cb=[obj{std::move(obj)},callback{std::move(callback)}](auto ctx, const Error& ec)
            {
                if (ec)
                {
                    callback(std::move(ctx),ec,du::ObjectId{});
                }
                else
                {
                    callback(std::move(ctx),Error{},obj->field(object::_id).value());
                }
            };
            contextDb(ctx).dbClient(topic)->create(
                std::move(ctx),
                std::move(cb),
                topic,
                model,
                unit
            );
        }

        template <typename ModelT, typename QueryBuilderWrapperT>
        static void list(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            const ModelT& model,
            QueryBuilderWrapperT query,
            Topic topic
        )
        {
            auto cb=[callback{std::move(callback)}](auto ctx, Result<common::pmr::vector<DbObject>> r)
            {
                std::cout << "r size " << r->size() << std::endl;
                callback(std::move(ctx),std::move(r));
            };
            contextDb(ctx).dbClient(query.threadTopic())->find(
                std::move(ctx),
                std::move(cb),
                model,
                std::move(query),
                topic
            );
        }

        template <typename ModelT>
        static void update(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const ModelT& model,
            const du::ObjectId& id,
            common::SharedPtr<update::Request> request,
            Topic topic
        )
        {
            contextDb(ctx).dbClient(topic)->find(
                std::move(ctx),
                callback,
                topic,
                model,
                id,
                request
            );
        }

        template <typename ModelT>
        static void update(
                common::SharedPtr<Context> ctx,
                CallbackEc callback,
                const ModelT& model,
                const du::ObjectId& id,
                common::SharedPtr<update::Request> request,
                Topic topic,
                common::Date date
            )
        {
            contextDb(ctx).dbClient(topic)->find(
                std::move(ctx),
                callback,
                topic,
                model,
                id,
                request,
                date
            );
        }

        template <typename ModelT>
        static void remove(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const ModelT& model,
            const du::ObjectId& id,
            Topic topic
        )
        {
            contextDb(ctx).dbClient(topic)->deleteObject(
                std::move(ctx),
                callback,
                topic,
                model,
                id
            );
        }

        template <typename ModelT>
        static void remove(
                common::SharedPtr<Context> ctx,
                CallbackEc callback,
                const ModelT& model,
                const du::ObjectId& id,
                Topic topic,
                common::Date date
            )
        {
            contextDb(ctx).dbClient(topic)->deleteObject(
                std::move(ctx),
                callback,
                topic,
                model,
                id,
                date
            );
        }

    private:

        static auto& contextDb(const common::SharedPtr<Context>& ctx)
        {
            return ContextTraits::contextDb(ctx);
        }
};

HATN_DB_NAMESPACE_END

#endif // HATNASYNCMODELCONTROLLER_H
