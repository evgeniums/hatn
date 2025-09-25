/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/

/** @file common/pointers/mempool/sharedptr.h
  *
  *     Shared pointer for managed object
  *
  */

/****************************************************************************/

#ifndef HATNMANAGESHAREDPTR_MP_H
#define HATNMANAGESHAREDPTR_MP_H

#include <memory>
#include <atomic>

#include <hatn/common/common.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/utils.h>

#include <hatn/common/pointers/mempool/managedobject.h>
#include <hatn/common/pointers/mempool/weakctrl.h>
#include <hatn/common/pointers/mempool/weakpool.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pointers_mempool {

//! Traits for intrusive and not intrusive shared pointer
template <typename T, typename=void> struct SharedPtrTraits
{
};
template <typename T> struct SharedPtrTraits<T,std::enable_if_t<std::is_base_of<ManagedObject,T>::value>>
{
    template <typename T1> inline static ManagedObject* m(T1* ptr) noexcept
    {
        return ptr->d;
    }
    template <typename T1> inline static void destroy(T1* ptr)
    {
        auto memoryResource=ptr->d->memoryResource();
        if (memoryResource==nullptr)
        {
            // object was created with new, use delete

            // codechecker_false_positive [cplusplus.NewDelete]
            delete ptr->d;
        }
        else
        {
            // object was allocated, use deallocation

            auto typeInfo=ptr->d->typeInfo();
            lib::destroyAt(static_cast<ManagedObject*>(ptr->d));
            memoryResource->deallocate(ptr->d,typeInfo.first,typeInfo.second);
        }
    }
};
template <typename T> struct SharedPtrTraits<T,std::enable_if_t<!std::is_base_of<ManagedObject,T>::value>>
{
    template <typename T1> inline static ManagedObject* m(T1* ptr) noexcept
    {
        return ptr->m;
    }

    template <typename T1> inline static void destroy(T1* ptr)
    {
        auto memoryResource=ptr->m->memoryResource();
        if (memoryResource==nullptr)
        {
            // object was created as single wrapper object with new, just delete it
            delete ptr->d;
            return;
        }

        auto typeInfo=ptr->m->typeInfo();
        if (typeInfo.first!=sizeof(ManagedObject))
        {
            // ManagedObject and underlying object are packed together into single wrapper object, destroy the wrapper
            lib::destroyAt(ptr->m);
            memoryResource->deallocate(ptr->m,typeInfo.first,typeInfo.second);
        }
        else
        {
            // ManagedObject and underlying object are separate objects, destroy them individually
            pmr::polymorphic_allocator<ManagedObject> allocator(ptr->m->memoryResource());
            pmr::destroyDeallocate(ptr->m,allocator);
            delete ptr->d;
        }
    }
};

//! Base class for intrusive and not intrusive shared pointers
/**
 *  This class can not be instantiated or destroyed directly. Use derived classes only.
 */
template <typename T> class SharedPtrBase
{
    public:

        using hana_tag=shared_pointer_tag;
        using element_type=T;

        //! Get held object const
        inline T* get() const noexcept
        {
            return d;
        }

        //! Get held object
        inline T* get() noexcept
        {
            return d;
        }

        //! Get held object const
        inline T* mutableValue() noexcept
        {
            return get();
        }

        //! Get held object const
        inline const T& value() const noexcept
        {
            return *d;
        }

        //! Operator -> const
        inline T* operator->() const noexcept
        {
            return d;
        }

        //! Operator ->
        inline T* operator->() noexcept
        {
            return d;
        }

        //! Operator * const
        inline T& operator*() const noexcept
        {
            return *d;
        }

        //! Operator *
        inline T& operator*() noexcept
        {
            return *d;
        }

        //! Check if ponter is null
        bool isNull() const noexcept
        {
            return d==nullptr;
        }

        //! Convert to bool
        inline operator bool() const noexcept
        {
            return !isNull();
        }

    protected:

        //! Ctor
        SharedPtrBase(T* obj=nullptr)  noexcept
            :d(obj)
        {
        }

        ~SharedPtrBase()=default;
        SharedPtrBase(const SharedPtrBase& other)=default;
        SharedPtrBase& operator=(const SharedPtrBase& other)=default;
        SharedPtrBase(SharedPtrBase&& other) noexcept : d(std::exchange(other.d,nullptr))
        {
        }
        SharedPtrBase& operator=(SharedPtrBase&& other) noexcept
        {
            if (&other!=this)
            {
                d=std::exchange(other.d,nullptr);
            }
            return *this;
        }

        T* d=nullptr;

        template <typename T1> static ManagedObject* m(T1* ptr) noexcept
        {
            return SharedPtrTraits<T>::m(ptr);
        }

        template <typename T1> static void reset(T1* ptr) noexcept
        {
            auto&& mm=SharedPtrTraits<T>::m(ptr);
            if (mm->refCount().fetch_sub(1, std::memory_order_acquire) == 1)
            {
                auto weak=mm->weakCtrl();
                if (weak!=nullptr)
                {
                    if (weak->lockForDelete())
                    {
                        weak->clearSharedPtr();
                        weak->reset();
                        SharedPtrTraits<T>::destroy(ptr);
                    }
                }
                else
                {
                    SharedPtrTraits<T>::destroy(ptr);
                }
            }
        }

        template <typename T1> static int refCount(T1* ptr) noexcept
        {
            auto mm=SharedPtrTraits<T>::m(ptr);
            int ret=0;
            if (ptr->d!=nullptr)
            {
                ret=mm->refCount();
            }
            return ret;
        }

        template <typename T1> static WeakCtrl* weakCtrl(T1* ptr) noexcept
        {
            WeakCtrl* ret=nullptr;
            if (ptr->d!=nullptr)
            {
                auto&& mm=SharedPtrTraits<T>::m(ptr);
                ret=mm->weakCtrl();
            }
            return ret;
        }

        template <typename T1> static void addRef(T1* ptr) noexcept
        {
            if (ptr->d!=nullptr)
            {
                auto&& mm=SharedPtrTraits<T>::m(ptr);
                mm->refCount().fetch_add(1, std::memory_order_release);
            }
        }

        template <typename U> friend class WeakPtr;
};

template <typename T> struct EmbeddedSharedPtr;

//! Shared pointer template
template <typename T, typename =void> class SharedPtr
{
};

//! Intrusive shared pointer
template <typename T> class SharedPtr<T,std::enable_if_t<std::is_base_of<ManagedObject,T>::value>>
        : public SharedPtrBase<T>
{
    public:

        using hana_tag=shared_pointer_tag;
        using element_type=T;

        //! Default ctor
        SharedPtr()
        {
            addRef();
        }

        //! Ctor from object
        explicit SharedPtr(T* obj, ManagedObject* =nullptr, pmr::memory_resource* resource=nullptr):SharedPtrBase<T>(obj)
        {
            setDestructionParameters(resource);
            addRef();
        }

        //! Copy ctor
        SharedPtr(const SharedPtr<T>& ptr) noexcept
            :SharedPtrBase<T>(ptr.d)
        {
            addRef();
        }

        //! Copy ctor
        SharedPtr(const EmbeddedSharedPtr<T>& ptr) noexcept
            :SharedPtrBase<T>(ptr.ptr)
        {
            addRef();
        }

        //! Ctor with implicit casting
        template <typename Y> SharedPtr(
                const SharedPtr<Y>& ptr
            )  noexcept
            : SharedPtr(ptr.template staticCast<T>())
        {}

        //! Assignment operator
        SharedPtr& operator=(const SharedPtr<T>& ptr) noexcept
        {
            if(this != &ptr)
            {
                reset(ptr.d);
            }
            return *this;
        }

        //! Assignment operator
        SharedPtr& operator=(const EmbeddedSharedPtr<T>& ptr)
        {
            reset(ptr.ptr,ptr.storage.get());
            return *this;
        }

        //! Move ctor
        SharedPtr(SharedPtr<T>&& other) noexcept
            :SharedPtrBase<T>(std::move(other))
        {
        }

        //! Move assignment operator
        SharedPtr& operator=(SharedPtr<T>&& other) noexcept
        {
            if(this != &other)
            {
                reset();
                this->d=other.d;
                other.d=nullptr;
            }
            return *this;
        }

        //! Dtor
        ~SharedPtr()
        {
            reset();
        }

        //! Swap pointers.
        void swap(SharedPtr<T>& other) noexcept
        {
            std::swap(this->d,other.d);
        }

        friend void swap(SharedPtr<T>& lhs, SharedPtr<T>& rhs)
        {
            std::swap(lhs.d,rhs.d);
        }

        //! Make dynamic cast
        template <typename Y>
        SharedPtr<Y> dynamicCast() const noexcept
        {
            auto casted=dynamic_cast<Y*>(this->d);
            if (casted==nullptr)
            {
                return SharedPtr<Y>{};
            }

            SharedPtr<Y> obj(casted,this->d);
            return obj;
        }

        //! Make static cast
        template <typename Y>
        SharedPtr<Y> staticCast() const noexcept
        {
            static_assert(std::is_base_of<Y,T>::value,"Y must be base type of T");

            SharedPtr<Y> obj(static_cast<Y*>(this->d),this->d);
            return obj;
        }

        //! Reset pointer
        inline void reset(T* obj=nullptr, ManagedObject* =nullptr, pmr::memory_resource* resource=nullptr)
        {
            if (this->d==obj)
            {
                return;
            }

            if (this->d!=nullptr)
            {
                SharedPtrBase<T>::reset(this);
            }

            this->d=obj;
            setDestructionParameters(resource);

            addRef();
        }

        //! Get ref count
        inline int refCount() const noexcept
        {
            return SharedPtrBase<T>::refCount(this);
        }

#ifndef HATN_TEST_SMART_POINTERS
    private:
#endif
        //! Get control block of weak pointer
        inline WeakCtrl* weakCtrl() const noexcept
        {
            return SharedPtrBase<T>::weakCtrl(this);
        }

    private:

        inline void addRef() noexcept
        {
            return SharedPtrBase<T>::addRef(this);
        }

        template <typename T1, typename> friend struct SharedPtrTraits;

        template <typename U> friend class WeakPtr;

        void setDestructionParameters(pmr::memory_resource* resource) noexcept
        {
            if (this->d==nullptr)
            {
                return;
            }

            // parameters of object destruction must be set only once per object in the first created SharedPtr
            if (this->d->typeInfo().first==0)
            {
                this->d->template setTypeInfo<T>();
            }
            if (resource!=nullptr && this->d->memoryResource()==nullptr)
            {
                this->d->setMemoryResource(resource);
            }
        }
};

//! Non-intrusive shared pointer
template <typename T> class SharedPtr<T,std::enable_if_t<!std::is_base_of<ManagedObject,T>::value>>
        : public SharedPtrBase<T>
{
    public:

        using hana_tag=shared_pointer_tag;
        using element_type=T;

        //! Default ctor
        SharedPtr() : SharedPtrBase<T>(nullptr),m(nullptr)
        {
            reset();
        }

        //! Ctor from object
        explicit SharedPtr(
                T* obj,
                ManagedObject* managed=nullptr,
                pmr::memory_resource* memoryResource=nullptr
            ):SharedPtrBase<T>(nullptr),m(nullptr)
        {
            reset(obj,managed,memoryResource);
        }

        //! Copy ctor
        SharedPtr(const SharedPtr& ptr) noexcept
            : SharedPtrBase<T>(ptr.d),m(ptr.m)
        {
            addRef();
        }

        //! Ctor with implicit casting
        template <typename Y> SharedPtr(
                const SharedPtr<Y>& ptr
            )  noexcept : SharedPtr(ptr.template staticCast<T>())
        {}

        //! Move ctor
        SharedPtr(SharedPtr&& other) noexcept
            :SharedPtrBase<T>(std::move(other)),
             m(other.m)
        {
            other.m=nullptr;
        }

        //! Copy ctor
        SharedPtr(const EmbeddedSharedPtr<T>& ptr) noexcept
            :SharedPtrBase<T>(ptr.obj),m(ptr.storage.get())
        {
            addRef();
        }

        //! Assignment operator
        SharedPtr& operator=(const SharedPtr& ptr)
        {
            if(this != &ptr)
            {
                reset(ptr.d,ptr.m);
            }
            return *this;
        }

        //! Move assignment operator
        SharedPtr& operator=(SharedPtr&& other) noexcept
        {
            if(this != &other)
            {
                reset();
                m=other.m;
                this->d=other.d;
                other.d=nullptr;
                other.m=nullptr;
            }
            return *this;
        }

        //! Assignment operator
        SharedPtr& operator=(const EmbeddedSharedPtr<T>& ptr)
        {
            reset(ptr.ptr,ptr.storage.get());
            return *this;
        }

        //! Dtor
        ~SharedPtr()
        {
            reset();
        }

        //! Swap pointers.
        void swap(SharedPtr<T>& other) noexcept
        {
            std::swap(this->d,other.d);
            std::swap(this->m,other.m);
        }

        //! Swap pointers.
        friend void swap(SharedPtr<T>& lhs, SharedPtr<T>& rhs) noexcept
        {
            lhs.swap(rhs);
        }

        //! Make dynamic cast
        template <typename Y>
        SharedPtr<Y> dynamicCast() const noexcept
        {
            auto casted=dynamic_cast<Y*>(this->d);
            if (casted==nullptr)
            {
                return SharedPtr<Y>{};
            }

            SharedPtr<Y> obj(casted,m);
            return obj;
        }

        //! Make static cast
        template <typename Y> SharedPtr<Y> staticCast() const noexcept
        {
            SharedPtr<Y> obj(static_cast<Y*>(this->d),m);
            return obj;
        }

        //! Make static cast
        SharedPtr<ManagedObject> staticCastM() const noexcept
        {
            return SharedPtr<ManagedObject>(m,m);
        }

        //! Reset pointer
        inline void reset(
                T* obj=nullptr,
                ManagedObject* managed=nullptr,
                pmr::memory_resource* memoryResource=nullptr
            )
        {
            if (this->d==obj)
            {
                if (m==managed)
                {
                    return;
                }
                else
                {
                    Assert(false,"It is forbidden to set new managed object for the same object");
                }
            }
            if (this->d!=nullptr)
            {
                SharedPtrBase<T>::reset(this);
            }
            this->d=obj;
            m=managed;
            if (this->d!=nullptr&&m==nullptr)
            {
                if (memoryResource==nullptr)
                {
                    memoryResource=pmr::get_default_resource();
                }
                pmr::polymorphic_allocator<ManagedObject> allocator(memoryResource);
                m=allocator.allocate(1);
                allocator.construct(m);

                m->setMemoryResource(memoryResource);
                m->template setTypeInfo<ManagedObject>();
            }
            addRef();
        }

        //! Get ref count
        inline int refCount() const noexcept
        {
            return SharedPtrBase<T>::refCount(this);
        }

#ifndef HATN_TEST_SMART_POINTERS
    private:
#endif
        //! Get contol block of weak pointer
        inline WeakCtrl* weakCtrl() const noexcept
        {
            return SharedPtrBase<T>::weakCtrl(this);
        }

    private:

        inline void addRef() noexcept
        {
            return SharedPtrBase<T>::addRef(this);
        }

        ManagedObject* m=nullptr;

        template <typename T1, typename> friend struct SharedPtrTraits;
        template <typename U> friend class WeakPtr;
};

//! Template class with sharedFromThis() method
template <typename T, bool=true>
class EnableSharedFromThis
{
    //! @todo Detect if T is derived from ManagedObject
};

template <typename T>
class EnableSharedFromThis<T,false>
{
    public:

        //! Make shared pointer from this object
        SharedPtr<T> sharedFromThis() const noexcept
        {
            auto self=const_cast<EnableSharedFromThis*>(this);
            return SharedPtr<T>(static_cast<T*>(self),self);
        }
};

template <typename T>
class EnableSharedFromThis<T,true> : public ManagedObject
{
    public:

    //! Make shared pointer from this object
    SharedPtr<T> sharedFromThis() const noexcept
    {
        // check if this object is in a SharedPtr
        if (this->refCount()==0)
        {
            return SharedPtr<T>{};
        }

        auto self=const_cast<EnableSharedFromThis*>(this);
        return SharedPtr<T>(static_cast<T*>(self),self);
    }
};

template <typename T>
class SharedFromThisWrapper : public T,
                              public EnableSharedFromThis<SharedFromThisWrapper<T>>
{
    public:

        using T::T;
};

//! SharedPtr to use in embedded fields in the same class T
template <typename T>
struct EmbeddedSharedPtr
{
    inline EmbeddedSharedPtr& operator=(const SharedPtr<T>& sharedPtr) noexcept
    {
        storage=sharedPtr;
        ptr=sharedPtr.get();
        return *this;
    }
    inline EmbeddedSharedPtr& operator=(SharedPtr<T>&& sharedPtr) noexcept
    {
        ptr=sharedPtr.get();
        storage=std::move(sharedPtr);
        return *this;
    }
    inline T* operator ->() noexcept
    {
        return ptr;
    }
    inline T* operator ->() const noexcept
    {
        return ptr;
    }
    inline void reset() noexcept
    {
        storage.reset();
        ptr=nullptr;
    }

    SharedPtr<T> castBack() const
    {
        return ptr->sharedFromThis();
    }

    T* ptr=nullptr;
    SharedPtr<ManagedObject> storage;
};

template <typename T1,typename=void> struct CheckSharedFromThis
{
};
template <typename T1> struct CheckSharedFromThis<T1,
                                std::enable_if_t<std::is_base_of<EnableSharedFromThis<T1>,T1>::value>>
{
    constexpr static const bool value=true;
    static SharedPtr<T1> createShared(T1* ptr)
    {
        return ptr->sharedFromThis();
    }
};
template <typename T1> struct CheckSharedFromThis<T1,
                                std::enable_if_t<!std::is_base_of<EnableSharedFromThis<T1>,T1>::value>>
{
    constexpr static const bool value=false;
    static SharedPtr<T1> createShared(T1*)
    {
        return SharedPtr<T1>();
    }
};

//---------------------------------------------------------------
        } // namespace pointers_mempool
HATN_COMMON_NAMESPACE_END
#endif // HATNMANAGESHAREDPTR_MP_H
