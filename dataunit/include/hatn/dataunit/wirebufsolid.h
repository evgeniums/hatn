/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/wirebufsolid.h
  *
  *  Contains traits for solid wire buf.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITWIREBUFSOLID_H
#define HATNDATAUNITWIREBUFSOLID_H

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/wirebuf.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

template <typename ImplT> class WireDataDerived;

template <typename ContainerHolderT>
struct WireBufSolidTraitsBase : public WireBufTraits,
                                public ContainerHolderT
{

    template <typename ...Args>
    WireBufSolidTraitsBase(Args ...args)
        : ContainerHolderT(std::forward<Args>(args)...),
          beginMain(true)
    {
    }

    constexpr static bool isSingleBuffer() noexcept
    {
        return true;
    }

    void clear()
    {
        beginMain=true;
        this->container().clear();
        WireBufTraits::clear();
    }

    void resetState()
    {
        beginMain=true;
        WireBufTraits::resetState();
    }

    template <typename T>
    int append(const T& other, const AllocatorFactory*)
    {
        int size=static_cast<int>(other.size());
        copyToContainer(other,mainContainer());
        return size;
    }

    common::ByteArray* mainContainer() const noexcept
    {
        return const_cast<common::ByteArray*>(&this->container());
    }

    common::DataBuf nextBuffer() const noexcept
    {
        if (beginMain)
        {
            beginMain=false;
            if (!this->container().isEmpty())
            {
                return common::DataBuf{this->container()};
            }
        }
        return common::DataBuf{};
    }

    void appendBuffer(common::SpanBuffer&& buf)
    {
        this->container().append(buf.view());
    }

    void appendBuffer(const common::SpanBuffer& buf)
    {
        this->container().append(buf.view());
    }

    void appendBuffer(common::DataBuf buf)
    {
        this->container().append(buf);
    }

    mutable bool beginMain;
};

struct ContainerOnStack
{
    ContainerOnStack(
        const AllocatorFactory* factory
        ) : m_container(factory->dataMemoryResource())
    {}

    ContainerOnStack(
        const char* data,
        size_t size,
        bool inlineBuffer
        ) : m_container(data,size,inlineBuffer)
    {}

    explicit ContainerOnStack(
        common::ByteArray container
        ) noexcept : m_container(std::move(container))
    {}

    common::ByteArray& container() noexcept
    {
        return m_container;
    }

    const common::ByteArray& container() const noexcept
    {
        return m_container;
    }

    common::ByteArrayShared sharedMainContainer() const noexcept
    {
        return common::ByteArrayShared{};
    }

    common::ByteArrayManaged* managedMainContainer() const noexcept
    {
        return nullptr;
    }

    common::ByteArray m_container;
};

struct ContainerShared
{
    ContainerShared(
        const AllocatorFactory* factory
        ) : m_container(factory->createObject<common::ByteArrayManaged>(factory->dataMemoryResource()))
    {}

    ContainerShared(
        common::ByteArrayShared container
        ) noexcept : m_container(std::move(container))
    {}

    common::ByteArray& container() noexcept
    {
        return *(m_container.get());
    }

    const common::ByteArray& container() const noexcept
    {
        return *(m_container.get());
    }

    common::ByteArrayShared sharedMainContainer() const noexcept
    {
        return m_container;
    }

    common::ByteArrayManaged* managedMainContainer() const noexcept
    {
        return const_cast<ContainerShared*>(this)->m_container.get();
    }

    common::ByteArrayShared m_container;
};

struct ContainerOnStackRef
{
    ContainerOnStackRef(
            common::ByteArray& ref
        ) : m_container(ref)
    {}

    common::ByteArray& container() noexcept
    {
        return m_container;
    }

    const common::ByteArray& container() const noexcept
    {
        return m_container;
    }

    common::ByteArrayShared sharedMainContainer() const noexcept
    {
        return common::ByteArrayShared{};
    }

    common::ByteArrayManaged* managedMainContainer() const noexcept
    {
        return nullptr;
    }

    common::ByteArray& m_container;
};

struct ContainerOnStackConstRef
{
    ContainerOnStackConstRef(
        const common::ByteArray& ref
        ) : m_container(ref)
    {}

    const common::ByteArray& container() const noexcept
    {
        return m_container;
    }

    common::ByteArrayShared sharedMainContainer() const noexcept
    {
        return common::ByteArrayShared{};
    }

    common::ByteArrayManaged* managedMainContainer() const noexcept
    {
        return nullptr;
    }

    const common::ByteArray& m_container;
};

using WireBufSolidTraits=WireBufSolidTraitsBase<ContainerOnStack>;
using WireBufSolidSharedTraits=WireBufSolidTraitsBase<ContainerShared>;
using WireBufSolidRefTraits=WireBufSolidTraitsBase<ContainerOnStackRef>;
using WireBufSolidConstRefTraits=WireBufSolidTraitsBase<ContainerOnStackConstRef>;

class HATN_DATAUNIT_EXPORT WireBufSolid : public WireBuf<WireBufSolidTraits>
{
    public:

        explicit WireBufSolid(
            const AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) : WireBuf<WireBufSolidTraits>(WireBufSolidTraits{factory},factory)
        {}

        explicit WireBufSolid(
            common::ByteArray container,
            const AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) noexcept
            : WireBuf<WireBufSolidTraits>(WireBufSolidTraits{std::move(container)},0,factory)
        {
            setSize(traits().container().size());
        }

        WireBufSolid(
            const char* data,
            size_t size,
            bool inlineBuffer,
            const AllocatorFactory* factory=AllocatorFactory::getDefault()
            ) : WireBuf<WireBufSolidTraits>(WireBufSolidTraits{data,size,inlineBuffer},size,factory)
        {}

        WireBufSolid(
            const char* data,
            size_t size,
            const AllocatorFactory* factory=AllocatorFactory::getDefault()
            ) : WireBuf<WireBufSolidTraits>(WireBufSolidTraits{data,size,false},size,factory)
        {}

        void load(
            const char* data,
            size_t size
            )
        {
            setSize(size);
            mainContainer()->load(data,size);
        }

        void loadInline(
            const char* data,
            size_t size
            )
        {
            setSize(size);
            mainContainer()->loadInline(data,size);
        }

        ~WireBufSolid()=default;
        WireBufSolid(const WireBufSolid&)=default;
        WireBufSolid(WireBufSolid&&)=default;
        WireBufSolid& operator=(const WireBufSolid&)=default;
        WireBufSolid& operator=(WireBufSolid&&)=default;

        template <typename T>
        WireBufSolid(
            WireDataDerived<T>&& buf
        );
};

class HATN_DATAUNIT_EXPORT WireBufSolidShared : public WireBuf<WireBufSolidSharedTraits>
{
    public:

        explicit WireBufSolidShared(
            const AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) : WireBuf<WireBufSolidSharedTraits>(WireBufSolidSharedTraits{factory},factory)
        {}

        explicit WireBufSolidShared(
            common::ByteArrayShared container,
            const AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) noexcept
            : WireBuf<WireBufSolidSharedTraits>(WireBufSolidSharedTraits{std::move(container)},0,factory)
        {
            setSize(traits().container().size());
        }

        ~WireBufSolidShared()=default;
        WireBufSolidShared(const WireBufSolidShared&)=delete;
        WireBufSolidShared(WireBufSolidShared&&)=default;
        WireBufSolidShared& operator=(const WireBufSolidShared&)=delete;
        WireBufSolidShared& operator=(WireBufSolidShared&&)=default;
};

class HATN_DATAUNIT_EXPORT WireBufSolidRef : public WireBuf<WireBufSolidRefTraits>
{
public:

    explicit WireBufSolidRef(
            common::ByteArray& container,
            const AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) noexcept
        : WireBuf<WireBufSolidRefTraits>(std::ref(container),0,factory)
    {
        setSize(traits().container().size());
    }

    ~WireBufSolidRef()=default;
    WireBufSolidRef(const WireBufSolidRef&)=delete;
    WireBufSolidRef(WireBufSolidRef&&)=default;
    WireBufSolidRef& operator=(const WireBufSolidRef&)=delete;
    WireBufSolidRef& operator=(WireBufSolidRef&&)=default;
};

class HATN_DATAUNIT_EXPORT WireBufSolidConstRef : public WireBuf<WireBufSolidConstRefTraits>
{
public:

    explicit WireBufSolidConstRef(
        const common::ByteArray& container,
        const AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) noexcept
        : WireBuf<WireBufSolidConstRefTraits>(std::cref(container),0,factory)
    {
        setSize(traits().container().size());
    }

    ~WireBufSolidConstRef()=default;
    WireBufSolidConstRef(const WireBufSolidConstRef&)=delete;
    WireBufSolidConstRef(WireBufSolidConstRef&&)=default;
    WireBufSolidConstRef& operator=(const WireBufSolidConstRef&)=delete;
    WireBufSolidConstRef& operator=(WireBufSolidConstRef&&)=default;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREBUFSOLID_H
