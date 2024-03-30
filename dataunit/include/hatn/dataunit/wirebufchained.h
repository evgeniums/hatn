/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/wirebufchained.h
  *
  *  Contains traits for chained wire buf.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITWIREBUFCHAINED_H
#define HATNDATAUNITWIREBUFCHAINED_H

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/wirebufsolid.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

struct WireBufChainedTraits : public WireBufSolidSharedTraits
{
    WireBufChainedTraits(
        AllocatorFactory* factory
    ) : WireBufSolidSharedTraits(factory),
        m_meta(factory->dataMemoryResource()),
        m_currentMainContainer(nullptr),
        m_bufferChain(factory->objectAllocator<common::ByteArrayShared>()),
        m_cursor(m_bufferChain.end())
    {}

    constexpr static bool isSingleBuffer() noexcept
    {
        return false;
    }

    common::ByteArray* appendMetaVar(size_t varTypeSize)
    {
        // packed size is +1 of var's type size
        ++varTypeSize;

        // check if the meta has enough capacity
        if (m_meta.capacity()<(m_meta.size()+varTypeSize))
        {
            // reserve meta capacity for more 16 variables in advance
            m_meta.reserve(m_meta.capacity()+varTypeSize*16);
            m_bufferChain.reserve(m_bufferChain.capacity()+32); // twice of m_meta count because 1 is for meta and 1 is for actual data

            // fix old pointers in span buffers
            for (auto& it:m_bufferChain)
            {
                if (!it.isSharedBuf())
                {
                    it.setData(m_meta.data());
                }
            }
        }

        auto offset=m_meta.size();
        m_meta.resize(offset+varTypeSize);

        // create buffer and append it to chain
        m_bufferChain.push_back(common::SpanBuffer(m_meta,offset,varTypeSize));
        m_cursor=m_bufferChain.begin();

        // return inline buffer
        m_tmpInline.loadInline(m_meta.data()+offset,varTypeSize);
        m_tmpInline.clear();
        return &m_tmpInline;
    }

    void setActualMetaVarSize(size_t actualVarSize)
    {
        auto& buf=m_bufferChain.back();
        buf.setSpanSize(actualVarSize);
    }

    void clear()
    {
        m_bufferChain.clear();
        m_meta.reset();
        m_tmpInline.reset();
        m_cursor=m_bufferChain.end();
        WireBufSolidSharedTraits::clear();
    }

    void resetState()
    {
        if (!m_bufferChain.empty())
        {
            m_cursor=m_bufferChain.begin();
        }
        else
        {
            m_cursor=m_bufferChain.end();
        }
        WireBufSolidSharedTraits::resetState();
    }

    void appendBuffer(const common::SpanBuffer& buf)
    {
        m_bufferChain.push_back(buf);
        m_cursor=m_bufferChain.begin();
    }

    void appendBuffer(common::SpanBuffer&& buf)
    {
        m_bufferChain.push_back(std::move(buf));
        m_cursor=m_bufferChain.begin();
    }

    common::SpanBuffer nextBuffer() const noexcept
    {
        if (m_cursor!=m_bufferChain.end())
        {
            return *m_cursor++;
        }
        return common::SpanBuffer();
    }

    void reserveMetaCapacity(size_t capacity)
    {
        m_meta.reserve(capacity);
    }

    common::ByteArray m_meta;
    mutable common::ByteArray m_tmpInline;
    common::ByteArray* m_currentMainContainer;

    common::SpanBuffers m_bufferChain;
    mutable decltype(m_bufferChain)::iterator m_cursor;
};

class HATN_DATAUNIT_EXPORT WireBufChained : public WireBuf<WireBufChainedTraits>
{
public:

    explicit WireBufChained(
        AllocatorFactory* factory=AllocatorFactory::getDefault()
    ) : WireBuf<WireBufChainedTraits>(WireBufChainedTraits{factory},factory)
    {}
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREBUFCHAINED_H
