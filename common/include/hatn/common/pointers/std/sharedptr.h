/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/pointers/std/sharedptr.h
  *
  *     Shared pointer for managed object
  *
  */

/****************************************************************************/

#ifndef HATNMANAGESHAREDPTR_STD_H
#define HATNMANAGESHAREDPTR_STD_H

#include <memory>

#include <hatn/common/common.h>
#include <hatn/common/utils.h>

#include <hatn/common/pointers/std/managedobject.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pointers_std {

template <typename T, typename=void> struct DeleteHelper
{
};
template <typename T> struct DeleteHelper<T,std::enable_if_t<std::is_base_of<ManagedObject,T>::value>>
{
    static void del(T* obj) noexcept
    {
        if (obj!=nullptr)
        {
            auto resource=obj->memoryResource();
            if (resource!=nullptr)
            {
                pmr::polymorphic_allocator<T> allocator(resource);
                pmr::destroyDeallocate(obj,allocator);
            }
            else
            {
                // codechecker_false_positive [cplusplus.NewDelete]
                delete obj;
            }
        }
    }
};
template <typename T> struct DeleteHelper<T,std::enable_if_t<!std::is_base_of<ManagedObject,T>::value>>
{
    static void del(T* obj) noexcept
    {
        delete obj;
    }
};

//! Shared pointer
template <typename T> class SharedPtr
{
    public:

        using hana_tag=shared_pointer_tag;

        //! Ctor from object
        explicit SharedPtr(T* obj=nullptr):p(obj,&DeleteHelper<T>::del)
        {
        }

        //! Move ctor
        SharedPtr(SharedPtr<T>&& ptr) noexcept
            :p(std::move(ptr.p))
        {
        }

        //! Copy ctor
        SharedPtr(const SharedPtr<T>& ptr)  noexcept
            :p(ptr.p)
        {
        }

        //! Ctor with implicit casting
        template <typename Y> SharedPtr(
                const SharedPtr<Y>& ptr
            ) noexcept : p(std::static_pointer_cast<T>(ptr.p))
        {}

        //! Move ctor from std::shared_ptr
        SharedPtr(std::shared_ptr<T>&& ptr) noexcept
            :p(std::move(ptr))
        {
        }

        //! Assignment operator
        SharedPtr& operator=(const SharedPtr<T>& ptr) noexcept
        {
            if(this != &ptr)
            {
               p=ptr.p;
            }            
            return *this;
        }

        //! Assignment operator
        SharedPtr& operator=(SharedPtr<T>&& ptr) noexcept
        {
            if(this != &ptr)
            {
                p=std::move(ptr.p);
            }
            return *this;
        }

        //! Dtor
        ~SharedPtr()
        {
            reset();
        }

        //! Make dynamic cast
        template <typename Y> SharedPtr<Y> dynamicCast() const noexcept
        {
            SharedPtr<Y> obj;
            obj.p=std::dynamic_pointer_cast<Y>(this->p);
            return obj;
        }

        //! Make static cast
        template <typename Y> SharedPtr<Y> staticCast() const noexcept
        {
            SharedPtr<Y> obj;
            obj.p=std::static_pointer_cast<Y>(this->p);
            return obj;
        }

        //! Reset pointer
        inline void reset(T* obj)
        {
            p.reset(obj,&DeleteHelper<T>::del);
        }

        //! Reset pointer
        inline void reset() noexcept
        {
            p.reset();
        }

        //! Get ref count
        inline int refCount() const noexcept
        {
            return p.use_count();
        }

        //! Get held object const
        inline T* get() const noexcept
        {
            return p.get();
        }

        //! Get held object noexcept
        inline T* get()
        {
            return p.get();
        }

        //! Get held object const
        inline T* mutableValue() noexcept
        {
            return get();
        }

        //! Get held object const
        inline const T& value() const noexcept
        {
            return *p;
        }

        //! Operator -> const
        inline T* operator->() const noexcept
        {
            return p.get();
        }

        //! Operator ->
        inline T* operator->() noexcept
        {
            return p.get();
        }

        //! Operator * const
        inline T& operator*() const noexcept
        {
            return *p;
        }

        //! Operator *
        inline T& operator*() noexcept
        {
            return *p;
        }

        //! Check if ponter is null
        inline bool isNull() const noexcept
        {
            return !static_cast<bool>(p);
        }

        //! Convert to bool
        inline operator bool() const noexcept
        {
            return static_cast<bool>(p);
        }

    private:

        std::shared_ptr<T> p;
        template <typename U> friend class WeakPtr;
        template <typename U> friend class SharedPtr;
};

//! Template class with method of creating shared pointer from this object
template <typename T, bool=true> class EnableSharedFromThis
{
};

template <typename T> class EnableSharedFromThis<T,true> : public ManagedObject, public std::enable_shared_from_this<T>
{
    public:

        /**
         * @brief Make shared pointer from this object
         * @return Shared pointer
         *
         * @note Can not be called from constructor
         */
        inline SharedPtr<T> sharedFromThis() noexcept
        {
            return SharedPtr<T>(this->shared_from_this());
        }
};

template <typename T> class EnableSharedFromThis<T,false> : public std::enable_shared_from_this<T>
{
    public:

        /**
         * @brief Make shared pointer from this object
         * @return Shared pointer
         *
         * @note Can not be called from constructor
         */
        inline SharedPtr<T> sharedFromThis() noexcept
        {
            return SharedPtr<T>(this->shared_from_this());
        }
};

//---------------------------------------------------------------
        } // namespace pointers_std
HATN_COMMON_NAMESPACE_END
#endif // HATNMANAGESHAREDPTR_STD_H
