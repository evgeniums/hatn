/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*
    
*/
/** @file common/objectguard.h
  *
  *     Guarded object that watches object existence
  *
  */

/****************************************************************************/

#ifndef HATNOBJECTGUARD_H
#define HATNOBJECTGUARD_H

#include <functional>

#include <hatn/common/common.h>

#include <hatn/common/managedobject.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/weakptr.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/pmr/pmrtypes.h>

HATN_COMMON_NAMESPACE_BEGIN

/**
 * @brief Base class for objects that can be NULL-guarded after destroying for use in asynchronous handlers.
 */
class HATN_COMMON_EXPORT WithGuard
{
    public:

        //! Ctor
        WithGuard():m_guard(allocateShared<ManagedObject>(
                                        pmr::polymorphic_allocator<ManagedObject>(memoryResource())
                                    ))
        {
        }

        //! Get weak guard
        inline WeakPtr<ManagedObject> guard() const noexcept
        {
            return m_guard;
        }

        //! Get memory resource for guards allocation
        inline static pmr::memory_resource* memoryResource() noexcept
        {
            return m_memoryResource?m_memoryResource:pmr::get_default_resource();
        }

        //! Set memory resource for guards allocation
        inline static void setMemoryResource(pmr::memory_resource* memoryResource) noexcept
        {
            m_memoryResource=memoryResource;
        }

        template <typename T>
        auto guardedAsyncHandler(T handler)
        {
            auto&& objectGuard=guard();
            return [objectGuard{std::move(objectGuard)},handler{std::move(handler)}](auto&&... args)
            {
                if (objectGuard.isNull())
                {
                    return;
                }
                auto g=objectGuard.lock();
                handler(std::forward<decltype(args)>(args)...);
            };
        }

    private:

        SharedPtr<ManagedObject> m_guard;
        static pmr::memory_resource* m_memoryResource;
};

class ObjectGuard
{
    public:

        template <typename T, typename HandlerT>
        auto guardedAsyncHandler(HandlerT handler, const SharedPtr<T>& object)
        {
            WeakPtr<T> guard{object};
            return [guard{std::move(guard)},handler{std::move(handler)}](auto&&... args)
            {
                auto object=guard.lock();
                if (object.isNull())
                {
                    return;
                }
                handler(std::forward<decltype(args)>(args)...);
            };
        }
};

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END

#endif // HATNOBJECTGUARD_H
