/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/pmr/allocatorfactory.h
  *
  *  Factory of allocators for allocation of objects and data fields
  *
  */

/****************************************************************************/

#ifndef HATNCOMMONFALLOCATORACTORY_H
#define HATNCOMMONFALLOCATORACTORY_H

#include <memory>

#include <hatn/common/sharedptr.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pmr/poolmemoryresource.h>
#include <hatn/common/pmr/withstaticallocator.h>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pmr {

namespace detail
{
template <typename T, typename = void>
struct ObjectCreatorTraits
{
};

template <typename T>
struct ObjectCreatorTraits<T,
                            std::enable_if_t<std::is_base_of<WithStaticAllocatorBase,T>::value>
                        >
{
    template <typename ... Args>
    static common::SharedPtr<T> f(common::pmr::memory_resource* factoryResource,Args&&... args)
    {
        auto resource=T::getStaticResource();
        if (resource==nullptr)
        {
            resource=factoryResource;
        }
        return common::allocateShared<T>(
                        common::pmr::polymorphic_allocator<T>(resource),
                        std::forward<Args>(args)...
                    );
    }
};

template <typename T>
struct ObjectCreatorTraits<T,
                            std::enable_if_t<!std::is_base_of<WithStaticAllocatorBase,T>::value>
                        >
{
    template <typename ... Args>
    static common::SharedPtr<T> f(common::pmr::memory_resource* factoryResource,Args&&... args)
    {
        return common::allocateShared<T>(
                        common::pmr::polymorphic_allocator<T>(factoryResource),
                        std::forward<Args>(args)...
                    );
    }
};

}

/**
 * @brief Factory of allocators for allocation of objects and data fields
 */
class HATN_COMMON_EXPORT AllocatorFactory final
{
    public:

        //! Get default allocation factory instance
        static const AllocatorFactory* getDefault() noexcept;

        //! Set default allocation factory instance
        static void setDefault(std::shared_ptr<AllocatorFactory> instance) noexcept;

        //! Get default memory resource
        static pmr::memory_resource* defaultDataMemoryResource() noexcept
        {
            return getDefault()->dataMemoryResource();
        }

        /**
         * @brief Reset default factory instance
         */
        static void resetDefault();

        /**
         * @brief Create factory using memory pools using default settings
         * @return Allocator factory
         */
        template <typename ObjectPoolT, typename DataPoolT>
        static std::shared_ptr<AllocatorFactory> createFactoryWithPools()
        {
            auto objectMemoryResource=std::make_shared<PoolMemoryResource<ObjectPoolT>>();
            auto dataPoolCacheGen=std::make_shared<memorypool::PoolCacheGen<DataPoolT>>();
            auto dataMemoryResource=std::make_shared<PoolMemoryResource<DataPoolT>>(dataPoolCacheGen);

            auto factory=std::make_shared<AllocatorFactory>(objectMemoryResource.get(),dataMemoryResource.get());
            factory->m_objectResource=objectMemoryResource;
            factory->m_dataResource=dataMemoryResource;
            return factory;
        }

        /**
         * @brief Default constructor
         *
         * Get memory resources from pmr::get_default_resource()
         */
        AllocatorFactory();

        /**
         * @brief Ctor
         * @param objectMemoryResource Memory resource for objects
         * @param dataMemoryResource() Memory resource for data
         * @param ownObjectResource Set this factory owner of object memory resource and delete the resource when factory is destroyed
         * @param ownDataResource Set this factory owner of data memory resource and delete the resource when factory is destroyed
         */
        AllocatorFactory(
            common::pmr::memory_resource* objectMemoryResource,
            common::pmr::memory_resource* dataMemoryResource,
            bool ownObjectResource=false,
            bool ownDataResource=false
        );

        /**
         * @brief Get object memory resource
         * @return Memory resource
         */
        inline common::pmr::memory_resource* objectMemoryResource() const noexcept
        {
            return m_objectResourceRef;
        }

        /**
         * @brief Get data memory resource
         * @return Memory resource
         */
        inline common::pmr::memory_resource* dataMemoryResource() const noexcept
        {
            return m_dataResourceRef;
        }

        //! Create object allocator
        template <typename T> inline common::pmr::polymorphic_allocator<T> objectAllocator() const noexcept
        {
            return common::pmr::polymorphic_allocator<T>(objectMemoryResource());
        }
        //! Create data allocator
        template <typename T> inline common::pmr::polymorphic_allocator<T> dataAllocator() const noexcept
        {
            return common::pmr::polymorphic_allocator<T>(dataMemoryResource());
        }
        //! Create bytes allocator
        inline common::pmr::polymorphic_allocator<char> bytesAllocator() const noexcept
        {
            return dataAllocator<char>();
        }

        //! Create object
        template <typename T, typename ...Args> common::SharedPtr<T> createObject(Args&&... args) const
        {
            return detail::ObjectCreatorTraits<T>::f(objectMemoryResource(),std::forward<Args>(args)...);
        }

        template <typename T>
        auto createDataVector() const
        {
            return vector<T>(dataAllocator<T>());
        }

        template <typename T>
        auto createObjectVector() const
        {            
            return vector<T>(objectAllocator<T>());
        }

        template <typename T>
        auto allocateObjectVector() const
        {
            using type=ManagedWrapper<pmr::vector<T>>;
            auto alloca=objectAllocator<T>();
            auto sharedPtrAlloca=objectAllocator<type>();
            auto buf=sharedPtrAlloca.allocate(1);
            auto ptr=new (buf) type(alloca);
            return SharedPtr<type>{ptr};
        }

        auto createString() const
        {
            return string(dataAllocator<string>());
        }

    private:

        common::pmr::memory_resource* m_objectResourceRef;
        common::pmr::memory_resource* m_dataResourceRef;

        std::shared_ptr<common::pmr::memory_resource> m_objectResource;
        std::shared_ptr<common::pmr::memory_resource> m_dataResource;
};

class WithFactory
{
    public:

        WithFactory(
                const AllocatorFactory* factory=AllocatorFactory::getDefault()
            ) : m_factory(factory)
        {}

        const AllocatorFactory* factory() const noexcept
        {
            return m_factory;
        }

        void setFactory(const AllocatorFactory* factory=AllocatorFactory::getDefault()) noexcept
        {
            m_factory=factory;
        }

    private:

        const AllocatorFactory* m_factory;
};

} // namespace pmr
HATN_COMMON_NAMESPACE_END
#endif // HATNCOMMONFALLOCATORACTORY_H
