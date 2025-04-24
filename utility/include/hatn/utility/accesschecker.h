/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/accesschecker.h
  */

/****************************************************************************/

#ifndef HATNUTILITYACLACCESSCHECKER_H
#define HATNUTILITYACLACCESSCHECKER_H

#include <hatn/common/allocatoronstack.h>
#include <hatn/db/modelsprovider.h>
#include <hatn/db/topic.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/aclconstants.h>
#include <hatn/utility/systemsection.h>
#include <hatn/utility/operation.h>

HATN_UTILITY_NAMESPACE_BEGIN

enum class AclStatus : uint8_t
{
    Unknown=0,
    Grant=1,
    Deny=2
};

struct HierarchyItem
{
    lib::string_view id;
    db::Topic topic;
};

struct HierarchyNone
{
    template <typename ContextT, typename CallbackT>
    void eachParent(
        common::SharedPtr<ContextT> ctx,
        CallbackT cb,
        lib::string_view,
        db::Topic
        )
    {
        cb(std::move(ctx),lib::optional<HierarchyItem>{},Error{},[](bool){});
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
            lib::string_view subject,
            db::Topic topic
        )
        {
            this->traits().eachParent(std::move(ctx),std::move(cb),subject,topic);
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
                lib::string_view object,
                db::Topic topic
            )
        {
            this->traits().eachParent(std::move(ctx),std::move(cb),object,topic);
        }
};

struct AccessCheckerArgs
{
    AccessCheckerArgs(
        lib::string_view object,
        lib::string_view objectTopic,
        lib::string_view subject,
        lib::string_view subjectTopic,
        const Operation* operation
        ) : object(object),
            subject(subject),
            operation(operation),
            objectTopic(objectTopic),
            subjectTopic(subjectTopic)
    {}

    ObjectType object;
    SubjectType subject;
    const Operation* operation;
    TopicType objectTopic;
    TopicType subjectTopic;

    lib::optional<uint32_t> objectMac;
    lib::optional<uint32_t> subjectMac;
};

struct MacPolicyNone
{
    template <typename ContextT, typename CallbackT>
    void checkMacPolicy(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            common::SharedPtr<AccessCheckerArgs> /*args*/
        )
    {
        cb(std::move(ctx),Error{});
    }
};

template <typename Traits=MacPolicyNone>
class MacPolicyChecker : public common::WithTraits<Traits>
{
public:

    using common::WithTraits<Traits>::WithTraits;

    template <typename ContextT, typename CallbackT>
    void checkMacPolicy(
        common::SharedPtr<ContextT> ctx,
        CallbackT cb,
        common::SharedPtr<AccessCheckerArgs> args
    )
    {
        this->traits().checkMacPolicy(std::move(ctx),std::move(cb),std::move(args));
    }
};


template <typename Traits>
class AccessCache : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        template <typename ContextT, typename CallbackT>
        void find(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            common::SharedPtr<AccessCheckerArgs> args,
            bool touch=true
        )
        {
            this->traits().find(std::move(ctx),std::move(cb),std::move(args),touch);
        }

        template <typename ContextT, typename CallbackT>
        void set(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            common::SharedPtr<AccessCheckerArgs> args,
            common::SharedPtr<AccessCheckerArgs> initialArgs,
            AclStatus status
            )
        {
            this->traits().find(std::move(ctx),std::move(cb),std::move(args),std::move(initialArgs),status);
        }
};

class AccessCacheNone
{
    public:

        template <typename ContextT, typename CallbackT>
        void find(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            common::SharedPtr<AccessCheckerArgs> /*args*/,
            bool /*touch*/
            )
        {
            cb(std::move(ctx),AclStatus::Unknown,Error{});
        }

        template <typename ContextT, typename CallbackT>
        void set(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            common::SharedPtr<AccessCheckerArgs> /*args*/,
            common::SharedPtr<AccessCheckerArgs> /*initialArgs*/,
            AclStatus /*status*/
            )
        {
            cb(std::move(ctx),Error{});
        }
};

template <typename SubjectHierarchyT=HierarchyNone, typename ObjectHierarchyT=HierarchyNone, typename CacheT=AccessCacheNone, typename MacPolicyCheckerT=MacPolicyNone>
struct AccessCheckerConfig
{
    using SubjectHierarchy=SubjectHierarchyT;
    using ObjectHierarchy=ObjectHierarchyT;
    using Cache=CacheT;
    using MacPolicyChecker=MacPolicyCheckerT;
};

template <typename ContextTraits, typename Config=AccessCheckerConfig<>>
class AccessChecker_p;

template <typename ContextTraits, typename Config=AccessCheckerConfig<>>
class AccessChecker
{
    public:

        using Context=typename ContextTraits::Context;
        using Callback=std::function<void (common::SharedPtr<Context>, AclStatus, const Error&)>;

        using SubjectHierarchy=typename Config::SubjectHierarchy;
        using ObjectHierarchy=typename Config::ObjectHierarchy;
        using Cache=typename Config::Cache;
        using MacPolicyChecker=typename Config::MacPolicyChecker;

        AccessChecker(
                std::shared_ptr<db::ModelsWrapper> wrapper,
                std::shared_ptr<SubjectHierarchy> subjHierarchy={},
                std::shared_ptr<ObjectHierarchy> objHierarchy={},
                std::shared_ptr<MacPolicyChecker> macPolicyChecker={}
            );

        AccessChecker(
                std::shared_ptr<db::ModelsWrapper> wrapper,
                std::shared_ptr<ObjectHierarchy> objHierarchy
            );

        AccessChecker(
                std::shared_ptr<SubjectHierarchy> subjHierarchy={},
                std::shared_ptr<ObjectHierarchy> objHierarchy={}
            );
        AccessChecker(
            std::shared_ptr<ObjectHierarchy> objHierarchy
        );

        ~AccessChecker();
        AccessChecker(const AccessChecker&)=delete;
        AccessChecker(AccessChecker&&)=default;
        AccessChecker& operator=(const AccessChecker&)=delete;
        AccessChecker& operator=(AccessChecker&&)=default;

        void setCache(std::shared_ptr<Cache> cache);

        void checkAccess(
            common::SharedPtr<Context> ctx,
            Callback callback,
            lib::string_view object,
            lib::string_view subject,
            const Operation* operation,
            lib::string_view objectTopic=SystemTopic,
            lib::string_view subjectTopic=SystemTopic
        ) const;

        void checkAccess(
            common::SharedPtr<Context> ctx,
            Callback callback,
            common::SharedPtr<AccessCheckerArgs> args,
            common::SharedPtr<AccessCheckerArgs> initialArgs={}
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

        std::shared_ptr<AccessChecker_p<ContextTraits,Config>> d;

        template <typename ContextTraits1, typename Config1>
        friend class AccessChecker_p;
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYACLACCESSCHECKER_H
