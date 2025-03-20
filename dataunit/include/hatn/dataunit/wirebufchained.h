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

#include <boost/hana.hpp>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/wirebufsolid.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

struct WireBufChainedTraits : public WireBufSolidSharedTraits
{
    WireBufChainedTraits(
        const AllocatorFactory* factory
    ) : WireBufSolidSharedTraits(factory),
        m_meta(factory->dataMemoryResource()),
        m_currentMainContainer(nullptr),
        m_buffers(factory->objectAllocator<common::SpanBuffer>()),
        m_cursor(m_chain.end())
    {
    }

    constexpr static bool isSingleBuffer() noexcept
    {
        return false;
    }

    static void rebindMeta(char* prevPtr, char* newPtr, std::vector<WireBufChainItem>& chain)
    {
        auto offset=newPtr-prevPtr;

        // fix old pointers in buffers
        for (auto& it:chain)
        {
            if (it.inMeta)
            {
                it.buf.rebind(it.buf.data()+offset);
            }
        }
    }

    common::DataBuf* appendMetaVar(size_t varTypeSize)
    {
        // packed size is +1 of var's type size
        ++varTypeSize;

        // check if the meta has enough capacity
        if (m_meta.capacity()<(m_meta.size()+varTypeSize))
        {
            auto prevPtr=m_meta.data();

            // reserve meta capacity for more 16 variables in advance
            m_meta.reserve(m_meta.capacity()+varTypeSize*16);
            m_chain.reserve(m_chain.capacity()+32); // twice of m_meta count because 1 is for meta and 1 is for actual data

            auto newPtr=m_meta.data();
            rebindMeta(prevPtr,newPtr,m_chain);
        }

        auto offset=m_meta.size();
        m_meta.resize(offset+varTypeSize);

        // create buffer and append it to chain
        appendBuffer(common::DataBuf{m_meta,offset,varTypeSize});

        // return appended buffer
        return &m_chain.back().buf;
    }

    void clear()
    {
        m_meta.clear();
        m_buffers.clear();
        m_chain.clear();
        m_cursor=m_chain.end();
        m_currentMainContainer=nullptr;
        WireBufSolidSharedTraits::clear();
    }

    void resetState()
    {
        if (!m_chain.empty())
        {
            m_cursor=m_chain.begin();
        }
        else
        {
            m_cursor=m_chain.end();
        }
        m_currentMainContainer=nullptr;
        WireBufSolidSharedTraits::resetState();
    }

    void appendBuffer(const common::SpanBuffer& buf)
    {
        auto view=buf.view();
        common::DataBuf dataBuf(view.data(),view.size());
        appendBufferFromSpan(std::move(dataBuf),buf.isSharedBuf());
        m_buffers.push_back(buf);
    }

    void appendBuffer(common::SpanBuffer&& buf)
    {
        auto view=buf.view();
        common::DataBuf dataBuf(view.data(),view.size());
        appendBufferFromSpan(std::move(dataBuf),buf.isSharedBuf());
        m_buffers.emplace_back(std::move(buf));
    }

    void appendBufferFromSpan(common::DataBuf buf, bool shared)
    {
        m_chain.emplace_back(std::move(buf),!shared);
        m_cursor=m_chain.begin();
    }

    void appendBuffer(common::DataBuf buf)
    {
        m_chain.emplace_back(std::move(buf));
        m_cursor=m_chain.begin();
    }

    common::SpanBuffers buffers() const
    {
        return m_buffers;
    }

    std::vector<WireBufChainItem> chain() const
    {
        return m_chain;
    }

    common::SpanBuffers chainBuffers(const AllocatorFactory* factory=AllocatorFactory::getDefault()) const
    {
        common::SpanBuffers bufs{factory->objectAllocator<common::SpanBuffer>()};
        auto size=m_chain.size();
        const auto* mainBuf=mainContainer();
        if (!mainBuf->empty())
        {
            size++;
        }
        bufs.reserve(size);
        if (!mainBuf->empty())
        {
            bufs.emplace_back(*mainBuf);
        }
        for (auto&& it: m_chain)
        {
            bufs.emplace_back(it.buf);
        }
        return bufs;
    }

    const common::ByteArray* meta() const
    {
        return &m_meta;
    }

    common::DataBuf nextBuffer() const noexcept
    {
        if (this->beginMain)
        {
            this->beginMain=false;
            auto main=this->managedMainContainer();
            if (main!=nullptr && !main->isEmpty())
            {
                return common::DataBuf{*main};
            }
        }

        if (m_cursor!=m_chain.end())
        {
            auto it=m_cursor++;
            return it->buf;
        }

        return common::DataBuf{};
    }

    common::ByteArray* mainContainer() const noexcept
    {
        return (m_currentMainContainer==nullptr)?this->managedMainContainer():m_currentMainContainer;
    }

    void setCurrentMainContainer(common::ByteArray* currentMainContainer) noexcept
    {
        m_currentMainContainer=currentMainContainer;
    }

    template <typename T>
    int append(const T& other, const AllocatorFactory* factory);

    common::ByteArray m_meta;
    common::ByteArray* m_currentMainContainer;

    common::SpanBuffers m_buffers;
    std::vector<WireBufChainItem> m_chain;
    mutable decltype(m_chain)::const_iterator m_cursor;
};

class HATN_DATAUNIT_EXPORT WireBufChained : public WireBuf<WireBufChainedTraits>
{
    public:

        explicit WireBufChained(
            const AllocatorFactory* factory=AllocatorFactory::getDefault(),
            bool useShareBuffers=false
        ) : WireBuf<WireBufChainedTraits>(WireBufChainedTraits{factory},factory,useShareBuffers)
        {}

        ~WireBufChained()=default;
        WireBufChained(const WireBufChained&)=delete;
        WireBufChained(WireBufChained&&)=default;
        WireBufChained& operator=(const WireBufChained&)=delete;
        WireBufChained& operator=(WireBufChained&&)=default;
};

template <typename T>
int WireBufChainedTraits::append(const T& other, const AllocatorFactory* factory)
{
    auto&& f=factory;

    // append other's main container regardless of buffer type
    auto* src=other.mainContainer();
    if (src!=nullptr && !src->isEmpty())
    {
        auto&& sharedBuf=f->template createObject<::hatn::common::ByteArrayManaged>(
            src->data(),src->size(),f->dataMemoryResource()
            );
        appendBuffer(common::SpanBuffer{std::move(sharedBuf)});
    }

    if (!other.isSingleBuffer())
    {
        // other is a chained buf, copy buffers

        // append buffers
        common::SpanBuffer::append(m_buffers,other.buffers());

        // make chain copy
        std::vector<WireBufChainItem> tmpChain{other.chain()};

        // copy meta
        const auto* otherMeta=other.meta();
        if (otherMeta!=nullptr && !otherMeta->isEmpty())
        {
            auto&& sharedBuf=f->template createObject<::hatn::common::ByteArrayManaged>(
                otherMeta->data(),otherMeta->size(),f->dataMemoryResource()
                );
            rebindMeta(otherMeta->data(),sharedBuf->data(),tmpChain);
            appendBuffer(common::SpanBuffer{std::move(sharedBuf)});
        }

        // move chain copy to self chain
        m_chain.reserve(m_chain.size()+tmpChain.size());
        m_chain.insert(std::end(m_chain),std::make_move_iterator(std::begin(tmpChain)),std::make_move_iterator(std::end(tmpChain)));
    }

    int size=static_cast<int>(other.size());
    return size;
}

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREBUFCHAINED_H
