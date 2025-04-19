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

template <typename Traits>
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

template <typename Traits>
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
};

template <typename Traits>
class AclController_p;

template <typename Traits>
class AclController
{
    public:

        using Context=typename Traits::Context;
        using Callback=std::function<void (common::SharedPtr<Context>, const Error&)>;

        using SubjectHierarchy=HATN_ACL_NAMESPACE::SubjectHierarchy<Traits>;
        using ObjectHierarchy=HATN_ACL_NAMESPACE::ObjectHierarchy<Traits>;
        using Cache=HATN_ACL_NAMESPACE::Cache<Traits>;

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
            return Traits::contextDb(ctx);
        }

        const common::pmr::AllocatorFactory* contextFactory(const common::SharedPtr<Context>& ctx) const
        {
            return Traits::contextFactory(ctx);
        }

        std::shared_ptr<AclController_p<Traits>> d;
};

HATN_ACL_NAMESPACE_END

#endif // HATNACLCONTROLLER_H
