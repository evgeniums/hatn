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
    struct ChainBuffer
    {
        ChainBuffer(
            common::DataBuf buf,
            bool sharedBuf=false
            ) : buf(buf),sharedBuf(sharedBuf)
        {}

        common::DataBuf buf;
        bool sharedBuf=false;
    };

    WireBufChainedTraits(
        AllocatorFactory* factory
    ) : WireBufSolidSharedTraits(factory),
        m_meta(factory->dataMemoryResource()),
        m_currentMainContainer(nullptr),
        m_buffers(factory->objectAllocator<common::ByteArrayShared>()),
        m_chain(factory->objectAllocator<common::ByteArrayShared>()),
        m_cursor(m_chain.end())
    {}

    constexpr static bool isSingleBuffer() noexcept
    {
        return false;
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
            auto offset=newPtr-prevPtr;

            // fix old pointers in buffers
            for (auto& it:m_chain)
            {
                if (!it.sharedBuf)
                {
                    it.buf.rebind(m_meta.data()+offset);
                }
            }
        }

        auto offset=m_meta.size();
        m_meta.resize(offset+varTypeSize);

        // create buffer and append it to chain
        appendBuffer(common::DataBuf(m_meta.data(),offset,varTypeSize));

        // return inline buffer
        return &m_chain.back();
    }

    void clear()
    {
        m_meta.reset();
        m_buffers.clear();
        m_chain.clear();
        m_cursor=m_chain.end();
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
        m_chain.emplace_back(std::move(buf),shared);
        m_cursor=m_chain.begin();
    }

    void appendBuffer(common::DataBuf buf)
    {
        m_chain.emplace_back(std::move(buf));
        m_cursor=m_chain.begin();
    }

    common::DataBuf nextBuffer() const noexcept
    {
        if (m_cursor!=m_chain.end())
        {
            auto it=m_cursor++;
            return it->buf;
        }
        return common::DataBuf{};
    }

    common::ByteArray* mainContainer() const noexcept
    {
        return (m_currentMainContainer==nullptr)?sharedMainContainer():m_currentMainContainer;
    }

    void setCurrentMainContainer(common::ByteArray* currentMainContainer) noexcept
    {
        m_currentMainContainer=currentMainContainer;
    }

    template <typename T>
    int append(const T& other, AllocatorFactory* factory);

    common::ByteArray m_meta;
    common::ByteArray* m_currentMainContainer;

    common::SpanBuffers m_buffers;
    common::pmr::vector<ChainBuffer> m_chain;
    mutable decltype(m_chain)::iterator m_cursor;
};

class HATN_DATAUNIT_EXPORT WireBufChained : public WireBuf<WireBufChainedTraits>
{
    public:

        using hana_tag=WireBufChainedTag;

        explicit WireBufChained(
            AllocatorFactory* factory=AllocatorFactory::getDefault()
        ) : WireBuf<WireBufChainedTraits>(WireBufChainedTraits{factory},factory)
        {}

    private:

        auto exportTrats() -> decltype(auto)
        {
            return traits();
        }
};


template <typename T>
int WireBufChainedTraits::append(const T& other, AllocatorFactory* factory)
{
    using otherT=std::decay_t<decltype(other)>;

    auto self=this;
    boost::hana::eval_if(
        boost::hana::is_a<WireBufChainedTag,otherT>,
        [&](auto _)
        {
            // other is a chained buf
            auto&& s=_(self);
            auto&& f=_(factory);
            auto&& o=_(other).exportTraits();

            // append buffers
            s->m_buffers.insert(std::end(s->m_buffers),std::begin(o.m_buffers),std::end(o.m_buffers));

            // copy meta
            if (!o.m_meta.isEmpty())
            {
                auto&& sharedBuf=f->template createObject<::hatn::common::ByteArrayManaged>(
                    o.m_meta.data(),o.m_meta.size(),f->dataMemoryResource()
                    );
                s->m_buffers.emplace_back(std::move(sharedBuf));
            }

            // append chain
            s->m_chain.insert(std::end(s->m_chain),std::begin(o.m_chain),std::end(o.m_chain));
        },
        [&](auto _)
        {
            // other must be a solid buf
            auto&& s=_(self);
            auto&& f=_(factory);

            auto* src=_(other).mainContainer();
            if (src!=nullptr && !src->isEmpty())
            {
                // allocated new shared buffer and copy prepared main container buffer to it
                auto&& sharedBuf=f->template createObject<::hatn::common::ByteArrayManaged>(
                    src->data(),src->size(),f->dataMemoryResource()
                    );
                s->appendBuffer(common::SpanBuffer{std::move(sharedBuf)});
            }
        }
        );

    int size=static_cast<int>(other->size());
    return size;
}

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREBUFCHAINED_H
