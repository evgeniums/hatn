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
#include <hatn/utility/accessstatus.h>
#include <hatn/utility/operation.h>
#include <hatn/utility/objectwrapper.h>

HATN_UTILITY_NAMESPACE_BEGIN

struct HierarchyNone
{
    template <typename ContextT, typename CallbackT>
    void eachParent(
        common::SharedPtr<ContextT> ctx,
        CallbackT cb,
        ObjectWrapperRef
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
            ObjectWrapperRef subject
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
                ObjectWrapperRef object
            )
        {
            this->traits().eachParent(std::move(ctx),std::move(cb),object);
        }
};

struct AccessCheckerArgs
{
    AccessCheckerArgs(
            ObjectWrapperRef object,
            ObjectWrapperRef subject,
            const Operation* operation
        ) : object(object),
            subject(subject),
            operation(operation)
    {}

    ObjectWrapper object;
    ObjectWrapper subject;

    const Operation* operation;

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
                AccessStatus status
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
            cb(std::move(ctx),AccessStatus::Unknown,Error{});
        }

        template <typename ContextT, typename CallbackT>
        void set(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            common::SharedPtr<AccessCheckerArgs> /*args*/,
            common::SharedPtr<AccessCheckerArgs> /*initialArgs*/,
            AccessStatus /*status*/
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
        using Callback=std::function<void (common::SharedPtr<Context>, AccessStatus, const Error&)>;

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

        void setSectionDbModelsWrapper(std::shared_ptr<db::ModelsWrapper> sectionDbModelsWrapper);

        ~AccessChecker();
        AccessChecker(const AccessChecker&)=delete;
        AccessChecker(AccessChecker&&)=default;
        AccessChecker& operator=(const AccessChecker&)=delete;
        AccessChecker& operator=(AccessChecker&&)=default;

        void setCache(std::shared_ptr<Cache> cache);

        void checkAccess(
            common::SharedPtr<Context> ctx,
            Callback callback,
            const Operation* operation,
            lib::string_view objectTopic=SystemTopic
        ) const;

        void checkAccess(
            common::SharedPtr<Context> ctx,
            Callback callback,
            ObjectWrapperRef object,
            const Operation* operation
        ) const;

        void checkAccess(
            common::SharedPtr<Context> ctx,
            Callback callback,
            ObjectWrapperRef object,
            ObjectWrapperRef subject,
            const Operation* operation
        ) const;

        void checkAccess(
            common::SharedPtr<Context> ctx,
            Callback callback,
            common::SharedPtr<AccessCheckerArgs> args,
            common::SharedPtr<AccessCheckerArgs> initialArgs={}
        ) const;

    private:

        std::shared_ptr<AccessChecker_p<ContextTraits,Config>> d;

        template <typename ContextTraits1, typename Config1>
        friend class AccessChecker_p;
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYACLACCESSCHECKER_H
