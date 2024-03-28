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

struct WireBufSolidTraits : public WireBufTraits
{
    constexpr static bool isSingleBuffer() noexcept
    {
        return true;
    }

    common::ByteArray* mainContainer() const noexcept
    {
        return const_cast<common::ByteArray*>(&m_container);
    }

    void clear()
    {
        m_container.clear();
    }

    private:

        common::ByteArray m_container;
};

class WireBufSolid : public WireBuf<WireBufSolidTraits>
{
    public:

        explicit WireBufSolid(
            AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) : WireBuf<WireBufSolidTraits>(factory),
            m_container(factory->dataMemoryResource())
        {}

        WireBufSolid(
            const char* data,
            size_t size,
            bool inlineBuffer,
            AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) : WireBuf<WireBufSolidTraits>(size,factory),
            m_container(data,size,inlineBuffer)
        {
            setUseInlineBuffers(inlineBuffer);
        }

        explicit WireBufSolid(
            common::ByteArray container,
            AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) noexcept
            : WireBuf<WireBufSolidTraits>(container.size(),factory),
              m_container(std::move(container))
        {}

    private:

        common::ByteArray m_container;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREBUFSOLID_H
