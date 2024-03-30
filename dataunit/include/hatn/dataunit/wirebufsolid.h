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

struct WireBufSolidTraits : public WireBufTraits
{
    WireBufSolidTraits(
        AllocatorFactory* factory
    ) : container(factory->dataMemoryResource())
    {}

    WireBufSolidTraits(
        const char* data,
        size_t size,
        bool inlineBuffer
    ) : container(data,size,inlineBuffer)
    {}

    explicit WireBufSolidTraits(
        common::ByteArray container
    ) noexcept : container(std::move(container))
    {}

    constexpr static bool isSingleBuffer() noexcept
    {
        return true;
    }

    common::ByteArray* mainContainer() const noexcept
    {
        return const_cast<common::ByteArray*>(&container);
    }

    void clear()
    {
        container.clear();
    }

    template <typename T>
    int append(const T& other, AllocatorFactory*)
    {
        int size=static_cast<int>(other->size());
        other.copyToContainer(mainContainer());
        return size;
    }

    common::ByteArray container;
};

class HATN_DATAUNIT_EXPORT WireBufSolid : public WireBuf<WireBufSolidTraits>
{
    public:

        using hana_tag=WireBufSolidTag;

        explicit WireBufSolid(
            AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) : WireBuf<WireBufSolidTraits>(WireBufSolidTraits{factory},factory)
        {}

        explicit WireBufSolid(
            common::ByteArray container,
            AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) noexcept
            : WireBuf<WireBufSolidTraits>(WireBufSolidTraits{std::move(container)},0,factory)
        {
            setSize(traits().container.size());
        }

        WireBufSolid(
            const char* data,
            size_t size,
            bool inlineBuffer,
            AllocatorFactory* factory=AllocatorFactory::getDefault()
            ) : WireBuf<WireBufSolidTraits>(WireBufSolidTraits{data,size,inlineBuffer},size,factory)
        {
            setUseInlineBuffers(inlineBuffer);
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

struct WireBufSolidSharedTraits : public WireBufTraits
{
    WireBufSolidSharedTraits(
        AllocatorFactory* factory
    ) : container(factory->createObject<common::ByteArrayManaged>(factory->dataMemoryResource()))
    {}

    WireBufSolidSharedTraits(
        common::ByteArrayShared container
    ) noexcept : container(std::move(container))
    {}

    constexpr static bool isSingleBuffer() noexcept
    {
        return true;
    }

    common::ByteArray* mainContainer() const noexcept
    {
        return sharedMainContainer();
    }

    common::ByteArray* sharedMainContainer() const noexcept
    {
        return const_cast<common::ByteArrayManaged*>(container.get());
    }

    void clear()
    {
        container->clear();
    }

    common::ByteArrayShared container;
};

class HATN_DATAUNIT_EXPORT WireBufSolidShared : public WireBuf<WireBufSolidSharedTraits>
{
    public:

        using hana_tag=WireBufSolidTag;

        explicit WireBufSolidShared(
            AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) : WireBuf<WireBufSolidSharedTraits>(WireBufSolidSharedTraits{factory},factory)
        {}

        explicit WireBufSolidShared(
            common::ByteArrayShared container,
            AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) noexcept
            : WireBuf<WireBufSolidSharedTraits>(WireBufSolidSharedTraits{std::move(container)},0,factory)
        {
            setSize(traits().container->size());
        }
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREBUFSOLID_H
