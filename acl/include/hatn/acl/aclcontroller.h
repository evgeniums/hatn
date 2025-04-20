/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file acl/aclcontroller.h
  */

/****************************************************************************/

#ifndef HATNACLCONTROLLER_H
#define HATNACLCONTROLLER_H

#include <hatn/common/allocatoronstack.h>
#include <hatn/db/modelsprovider.h>

#include <hatn/acl/acl.h>
#include <hatn/acl/aclconstants.h>

HATN_ACL_NAMESPACE_BEGIN

enum class AclStatus : uint8_t
{
    Unknown=0,
    Grant=1,
    Deny=2
};

struct HierarchyNone
{
    template <typename ContextT, typename CallbackT>
    void eachParent(
        common::SharedPtr<ContextT> ctx,
        CallbackT cb,
        lib::string_view
        )
    {
        cb(std::move(ctx),lib::optional<lib::string_view>{},Error{},[](bool){});
    }
};

template <typename Traits=HierarchyNone>
class SubjectHierarchy : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        template <typename ContextT, typename CallbackT>
        void eachParent(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            lib::string_view subject
        )
        {
            this->traits().eachParent(std::move(ctx),std::move(cb),subject);
        }
};

template <typename Traits=HierarchyNone>
class ObjectHierarchy : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        template <typename ContextT, typename CallbackT>
        void eachParent(
                common::SharedPtr<ContextT> ctx,
                CallbackT cb,
                lib::string_view object
            )
        {
            this->traits().eachParent(std::move(ctx),std::move(cb),object);
        }
};

struct AclControllerArgs
{
    AclControllerArgs(
        lib::string_view object,
        lib::string_view subject,
        lib::string_view operation,
        lib::string_view topic
    ) : object(object),
        subject(subject),
        operation(operation),
        topic(topic)
    {}

    ObjectType object;
    SubjectType subject;
    OperationType operation;
    TopicType topic;
};

template <typename Traits>
class Cache : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        template <typename ContextT, typename CallbackT>
        void find(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            common::SharedPtr<AclControllerArgs> args,
            bool touch=true
        )
        {
            this->traits().find(std::move(ctx),std::move(cb),std::move(args),touch);
        }

        template <typename ContextT, typename CallbackT>
        void set(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            common::SharedPtr<AclControllerArgs> args,
            common::SharedPtr<AclControllerArgs> initialArgs,
            AclStatus status
            )
        {
            this->traits().find(std::move(ctx),std::move(cb),std::move(args),std::move(initialArgs),status);
        }
};

class CacheNone
{
    public:

        template <typename ContextT, typename CallbackT>
        void find(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            common::SharedPtr<AclControllerArgs> /*args*/,
            bool /*touch*/
            )
        {
            cb(std::move(ctx),AclStatus::Unknown,Error{});
        }

        template <typename ContextT, typename CallbackT>
        void set(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            common::SharedPtr<AclControllerArgs> /*args*/,
            common::SharedPtr<AclControllerArgs> /*initialArgs*/,
            AclStatus /*status*/
            )
        {
            cb(std::move(ctx),Error{});
        }
};

template <typename ContextTraits, typename SubjectHierarchyT=HierarchyNone, typename ObjectHierarchyT=HierarchyNone, typename CacheT=CacheNone>
class AclController_p;

template <typename ContextTraits, typename SubjectHierarchyT=HierarchyNone, typename ObjectHierarchyT=HierarchyNone, typename CacheT=CacheNone>
class AclController
{
    public:

        using Context=typename ContextTraits::Context;
        using Callback=std::function<void (common::SharedPtr<Context>, AclStatus, const Error&)>;

        using SubjectHierarchy=SubjectHierarchyT;
        using ObjectHierarchy=ObjectHierarchyT;
        using Cache=CacheT;

        AclController(
                std::shared_ptr<db::ModelsWrapper> wrapper,
                std::shared_ptr<SubjectHierarchy> subjHierarchy={},
                std::shared_ptr<ObjectHierarchy> objHierarchy={}
            );
        AclController(
                std::shared_ptr<db::ModelsWrapper> wrapper,
                std::shared_ptr<ObjectHierarchy> objHierarchy
            );

        AclController(
                std::shared_ptr<SubjectHierarchy> subjHierarchy={},
                std::shared_ptr<ObjectHierarchy> objHierarchy={}
            );
        AclController(
            std::shared_ptr<ObjectHierarchy> objHierarchy
        );

        ~AclController();
        AclController(const AclController&)=delete;
        AclController(AclController&&)=default;
        AclController& operator=(const AclController&)=delete;
        AclController& operator=(AclController&&)=default;

        void setCache(std::shared_ptr<Cache> cache);

        void checkAccess(
            common::SharedPtr<Context> ctx,
            Callback callback,
            lib::string_view object,
            lib::string_view subject,
            lib::string_view operation,
            lib::string_view topic={}
        ) const;

        void checkAccess(
            common::SharedPtr<Context> ctx,
            Callback callback,
            common::SharedPtr<AclControllerArgs> args,
            common::SharedPtr<AclControllerArgs> initialArgs={}
        ) const;

    private:

        auto& contextDb(const common::SharedPtr<Context>& ctx) const
        {
            return ContextTraits::contextDb(ctx);
        }

        const common::pmr::AllocatorFactory* contextFactory(const common::SharedPtr<Context>& ctx) const
        {
            return ContextTraits::contextFactory(ctx);
        }

        std::shared_ptr<AclController_p<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>> d;

        template <typename ContextTraits1, typename SubjectHierarchyT1, typename ObjectHierarchyT1, typename CacheT1>
        friend class AclController_p;

};

HATN_ACL_NAMESPACE_END

#endif // HATNACLCONTROLLER_H
