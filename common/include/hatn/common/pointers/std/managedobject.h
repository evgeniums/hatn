/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file common/pointers/std/managedobject.h
  *
  *     Base class for managed objects stored in memory pools
  *
  */

/****************************************************************************/

#ifndef HATNMANAGEDOBJECT_STD_H
#define HATNMANAGEDOBJECT_STD_H

#include <memory>
#include <atomic>

#include <hatn/common/common.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/memorypool/rawblock.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pointers_std {

//! Base managed object
class HATN_COMMON_EXPORT ManagedObject
{
    public:

        //! Ctor
        ManagedObject() noexcept : m_memoryResource(nullptr)
        {}

        virtual ~ManagedObject()=default;
        ManagedObject(const ManagedObject&)=delete;
        ManagedObject(ManagedObject&&) =delete;
        ManagedObject& operator=(const ManagedObject&)=delete;
        ManagedObject& operator=(ManagedObject&&) =delete;

        //! Keep memory resource
        inline void setMemoryResource(pmr::memory_resource* resource) noexcept
        {
            m_memoryResource=resource;
        }
        //! Get allocator
        inline pmr::memory_resource* memoryResource() const noexcept
        {
            return m_memoryResource;
        }

    private:

        pmr::memory_resource* m_memoryResource;
};

template <typename T>
class EnableManaged : public ManagedObject
{
};

template <typename T>
class ManagedWrapper : public EnableManaged<ManagedWrapper<T>>, public T
{
public:

    using T::T;

    /** Get reference to self **/
    inline const EnableManaged<T>& value() const noexcept
    {
        return *this;
    }

    /** Get mutable pointer to self **/
    inline EnableManaged<T>* mutableValue() noexcept
    {
        return this;
    }
};

using ManagedObject=ManagedObject;

//---------------------------------------------------------------
        } // namespace std
    HATN_COMMON_NAMESPACE_END
#endif // HATNMANAGEDOBJECT_STD_H
