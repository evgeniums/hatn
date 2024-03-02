/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/*
    
*/
/** @file common/interface.h
 *
 *     Base class for container interfaces.
 *
 */
/****************************************************************************/

#ifndef HATNINTERFACE_H
#define HATNINTERFACE_H

#include <map>

#include <hatn/common/common.h>
#include <hatn/common/classuid.h>
#include <hatn/common/utils.h>
#include <hatn/common/locker.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Base class for interface classes
template<typename T> using Interface=hatn::common::ClassUid<T>;

//! Base class for multiple interface classes
template<typename T, typename ... Classes>
class MultiInterface : public Classes..., public hatn::common::ClassUid<T,Classes...>
{
    public:

        using hatn::common::ClassUid<T,Classes...>::cuid;
        using hatn::common::ClassUid<T,Classes...>::cuids;
        using hatn::common::ClassUid<T,Classes...>::fillCuids;
};

//! Base class for container of interfaces
class InterfacesContainer
{
    public:

        InterfacesContainer()=default;
        virtual ~InterfacesContainer()=default;
        InterfacesContainer(const InterfacesContainer&)=default;
        InterfacesContainer(InterfacesContainer&&) =default;
        InterfacesContainer& operator=(const InterfacesContainer&)=default;
        InterfacesContainer& operator=(InterfacesContainer&&) =default;

        //! Get interface
        template <typename T> T* interface() const noexcept
        {
            return reinterpret_cast<T*>(interfaceByUid(T::cuid()));
        }

        //! Get interface
        template <typename T> T* interfaceMapped() const noexcept
        {
            return reinterpret_cast<T*>(interfaceByUidMapped(T::cuid()));
        }

        //! Get interface by UID
        virtual void* interfaceByUid(CUID_TYPE uid) const noexcept
        {
            std::ignore=uid;
            return nullptr;
        }

        //! Get interface by UID using map
        virtual void* interfaceByUidMapped(CUID_TYPE uid) const noexcept
        {
            std::ignore=uid;
            return nullptr;
        }

        //! Get interface by position
        virtual void* interfaceAtPos(size_t pos) const noexcept
        {
            std::ignore=pos;
            return nullptr;
        }
};

//! Pack of interfaces
template <typename Tag,typename ...Classes> struct InterfacesPack
{};
template <typename Tag,typename Class> struct InterfacesPack<Tag,Class>
{
    Class interface;

    //! Get interface by UID
    inline void* interfaceByUid(CUID_TYPE uid) const noexcept
    {
        void* ret=nullptr;
        if (Class::cuid()==uid)
        {
            ret=const_cast<Class*>(&interface);
        }
        return ret;
    }

    //! Get interface by type
    template <typename T> void* interfaceByTypeVoid() const noexcept
    {
        if (std::is_same<T,Class>::value)
        {
            return const_cast<Class*>(&interface);
        }
        return nullptr;
    }
    //! Get interface by type
    template <typename T> T* interfaceByType() const noexcept
    {
        return reinterpret_cast<T*>(interfaceByTypeVoid<T>());
    }

    //! Get interface by position
    template <int N> void* interfaceByPos() const noexcept
    {
        if (N==0)
        {
            return const_cast<Class*>(&interface);
        }
        return nullptr;
    }

    //! Get interface at position
    inline void* interfaceAtPos(size_t pos, size_t sizeOfClasses=0) const noexcept
    {
        void* ret=nullptr;
        if (pos==sizeOfClasses)
        {
            ret=const_cast<Class*>(&interface);
        }
        return ret;
    }

    //! Add interface to map
    inline void addToMap(std::map<CUID_TYPE,uintptr_t>& m) const
    {
        const auto classCuids=Class::cuids();
        for (auto&& it: classCuids)
        {
            m[it]=reinterpret_cast<uintptr_t>(const_cast<Class*>(&interface));
        }
    }
};
template <typename Tag,typename Class,typename ...Classes> struct InterfacesPack<Tag,Class,Classes...>
{
    InterfacesPack<Tag,Classes...> interfaces;
    Class interface;

    //! Get interface by UID
    inline void* interfaceByUid(CUID_TYPE uid) const noexcept
    {
        void* ret=nullptr;
        if (Class::cuid()==uid)
        {
            ret=const_cast<Class*>(&interface);
        }
        else
        {
            ret=interfaces.interfaceByUid(uid);
        }
        return ret;
    }

    //! Get interface by type
    template <typename T> void* interfaceByTypeVoid() const noexcept
    {
        void* ret=nullptr;
        if (std::is_same<T,Class>::value)
        {
            ret=const_cast<Class*>(&interface);
        }
        else
        {
            ret=interfaces.template interfaceByType<T>();
        }
        return ret;
    }

    //! Get interface by type
    template <typename T> T* interfaceByType() const noexcept
    {
        return reinterpret_cast<T*>(interfaceByTypeVoid<T>());
    }

    //! Get interface by position
    template <int N> void* interfaceByPos() const noexcept
    {
        void* ret=nullptr;
        if (N==sizeof...(Classes))
        {
            ret=const_cast<Class*>(&interface);
        }
        else
        {
            ret=interfaces.template interfaceByPos<N-1>();
        }
        return ret;
    }

    //! Get interface at position
    inline void* interfaceAtPos(size_t pos, size_t sizeOfClasses=0) const noexcept
    {
        void* ret=nullptr;
        size_t sizeOfClassesI=sizeOfClasses;
        if (sizeof...(Classes)>sizeOfClassesI)
        {
            sizeOfClassesI=sizeof...(Classes);
        }
        if (pos==(sizeOfClassesI-sizeof...(Classes)))
        {
            ret=const_cast<Class*>(&interface);
        }
        else
        {
            ret=interfaces.interfaceAtPos(pos,sizeOfClassesI);
        }
        return ret;
    }

    //! Add interface to map
    inline void addToMap(std::map<CUID_TYPE,uintptr_t>& m) const
    {
        auto classCuids=Class::cuids();
        for (auto&& it: classCuids)
        {
            m[it]=reinterpret_cast<uintptr_t>(const_cast<Class*>(&interface));
        }
        interfaces.addToMap(m);
    }
};

//! Map of interfaces for fast access
template <typename T> class InterfacesMap
{
    public:

        /**
         * @brief Fill the map
         * @param pack
         *
         * We consider that offsets between interfaces are the same for all objects of the same type.
         * So, we just remember offsets in the map on the first call.
         */
        static inline void fill(const T& pack)
        {
            l.lock();
            if (m.empty())
            {
                const void* p=&pack;
                pack.addToMap(m);
                for (auto& it: m)
                {
                    it.second=it.second-reinterpret_cast<uintptr_t>(p);
                }
            }
            ready.store(true,std::memory_order_release);
            l.unlock();            
        }

        //! Get interface
        static inline void* interface(const T& base,CUID_TYPE cuid)
        {
            if (!ready.load(std::memory_order_acquire))
            {
                // lazy filling on first call
                fill(base);
            }
            auto it=m.find(cuid);
            if (it!=m.end())
            {
                const void* p=&base;
                return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(p)+it->second);
            }
            return nullptr;
        }

    private:

        static std::map<CUID_TYPE,uintptr_t> m;
        static MutexLock l;
        static std::atomic<bool> ready;
};
template <typename T> std::map<CUID_TYPE,uintptr_t> InterfacesMap<T>::m;
template <typename T> MutexLock InterfacesMap<T>::l;
template <typename T> std::atomic<bool> InterfacesMap<T>::ready(false);

//! Template class for class containing pack of interfaces
template <typename T,typename ...InterfaceClasses> class Interfaces : public T
{
    public:

        using Type=common::InterfacesPack<Interfaces, InterfaceClasses...>;

        //! Ctor
        template <typename ...Args> Interfaces(Args&& ... args) : T(std::forward<Args>(args)...)
        {
        }

        //! Get interface by type
        template <typename T1> constexpr T1* interface() const noexcept
        {
            return m_interfaces.template interfaceByType<T1>();
        }

        //! Get interface by position
        template <int N> constexpr void* interfaceByPos() const noexcept
        {
            return m_interfaces.template interfaceByPos<N>();
        }

        //! Get interface by UID of interface's class
        /**
         * Worst computational time is O(N), where N is a number of interfaces in the pack.
         * In default implementation we use sequential iteration over all interfaces in the pack.
         */
        virtual void* interfaceByUid(CUID_TYPE uid) const noexcept override
        {
            return m_interfaces.interfaceByUid(uid);
        }

        //! Get interface by position
        /**
         * Worst computational time is O(N), where N is a number of interfaces in the pack
         */
        virtual void* interfaceAtPos(size_t pos) const noexcept override
        {
            return m_interfaces.interfaceAtPos(pos);
        }

        //! Get interface by type using map of interfaces
        /**
         *  Worst computational time depends on search efficiency of std::map.
         *  This method is efficient only when there are a lot of interfaces in the pack and not efficient for small numbers of interfaces
         *  because it involves some locking overhead.
         */
        virtual void* interfaceByUidMapped(CUID_TYPE cuid) const noexcept override
        {
            return InterfacesMap<Type>::interface(m_interfaces,cuid);
        }

        //! Fill interfaces map
        inline void initInterfacesMap() const noexcept
        {
            initInterfacesMap(*this);
        }

        //! Fill interfaces map using sample
        static inline void initInterfacesMap(const Interfaces<T, InterfaceClasses...>& sample) noexcept
        {
            InterfacesMap<Type>::fill(sample.m_interfaces);
        }

    protected:

        Type m_interfaces;
};

//! Template of wrapper of an object with a single interface
template <typename T> class InterfaceWrapper : public common::Interface<T>
{
    public:

        inline void setObject(T* object) noexcept
        {
            m_object=object;
        }

        inline const T& object() const noexcept
        {
            return *m_object;
        }

        inline T* mutableObject() noexcept
        {
            return m_object;
        }

    private:

        T* m_object=nullptr;
};

//! Template of object with single interface
template <typename T, typename F> class SingleInterfaceClass : public Interfaces<T,F>
{
    public:

        using InterfaceWrapperType=F;

        //! Ctor
        template <typename ...Args> SingleInterfaceClass(Args&& ... args) : Interfaces<T,InterfaceWrapperType>(std::forward<Args>(args)...)
        {
            this->wrapper()->setObject(this);
        }

        virtual ~SingleInterfaceClass()=default;
        SingleInterfaceClass(const SingleInterfaceClass&)=default;
        SingleInterfaceClass(SingleInterfaceClass&&) =default;
        SingleInterfaceClass& operator=(const SingleInterfaceClass&)=default;
        SingleInterfaceClass& operator=(SingleInterfaceClass&&) =default;

        //! Get wrapper
        inline InterfaceWrapperType* wrapper() noexcept
        {
            return this->template interface<InterfaceWrapperType>();
        }

        //! Get wrapper
        inline InterfaceWrapperType* wrapper() const noexcept
		{
            return this->template interface<InterfaceWrapperType>();
		}

        inline const T& object() const noexcept
        {
            return wrapper()->object();
        }
        inline T* mutableObject() noexcept
        {
            return wrapper()->mutableObject();
        }
};

HATN_COMMON_NAMESPACE_END

#endif // HATNINTERFACE_H
