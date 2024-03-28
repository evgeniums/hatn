/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/wirebuf.h
  *
  *  Contains wire buffer class templates.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITWIREBUF_H
#define HATNDATAUNITWIREBUF_H

#include <hatn/common/bytearray.h>
#include <hatn/common/spanbuffer.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/allocatorfactory.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

struct WireBufTraits
{
    common::SpanBuffer nextBuffer() const noexcept {return common::SpanBuffer();}
    common::ByteArray* mainContainer() const noexcept {return nullptr;}
    void appendBuffer(common::SpanBuffer&& /*buf*/){}
    void appendBuffer(const common::SpanBuffer& /*buf*/){}

    // bool isSingleBuffer() const noexcept {return true;}

    void reserveMetaCapacity(size_t) {}
    void resetState() {m_offset=0;}
    void setCurrentMainContainer(common::ByteArray* /*currentMainContainer*/) noexcept
    {}

    common::ByteArray* appendMetaVar(size_t varTypeSize)
    {
        Assert(false,"Invalid operation");
        std::ignore=varTypeSize;
        return nullptr;
    }

    void clear(){}

    void setActualMetaVarSize(size_t actualVarSize)
    {
        Assert(false,"Invalid operation");
        std::ignore=actualVarSize;
    }

    size_t m_offset;
};

template <typename TraitsT>
class WireBuf
{    
    public:

        constexpr static bool isSingleBuffer() noexcept
        {
            return TraitsT::isSingleBuffer();
        }

        explicit WireBuf(
            AllocatorFactory* factory,
            bool useShareBuffers=false
        ) noexcept : WireBuf(0,factory,useShareBuffers)
        {}

        explicit WireBuf(
            size_t size=0,
            AllocatorFactory* factory=nullptr,
            bool useShareBuffers=false
        ) noexcept :
            m_size(size),
            m_factory(factory),
            m_useShareBuffers(useShareBuffers),
            m_useInlineBuffers(false)
        {}

#if __cplusplus < 201703L
        // to use as return in create() method
        WireBuf(const WireBuf&)=default;
#else
        WireBuf(const WireBuf&)=delete;
#endif
        WireBuf& operator=(const WireBuf&)=delete;
        WireBuf(WireBuf&&) =default;
        WireBuf& operator=(WireBuf&&) =default;

        // begin methods with traits

        common::SpanBuffer nextBuffer() const noexcept
        {
            return m_traits.nextBuffer();
        }

        common::ByteArray* mainContainer() const noexcept
        {
            return m_traits.mainContainer();
        }

        void appendBuffer(common::SpanBuffer&& buf)
        {
            m_traits.appendBuffer(std::move(buf));
        }

        void appendBuffer(const common::SpanBuffer& buf)
        {
            m_traits.appendBuffer(buf);
        }

        void setCurrentMainContainer(common::ByteArray* currentMainContainer) noexcept
        {
            m_traits.setCurrentMainContainer(currentMainContainer);
        }

        void reserveMetaCapacity(size_t size)
        {
            m_traits.reserveMetaCapacity(size);
        }

        void resetState()
        {
            m_traits.m_offset=0;
            m_traits.resetState();
        }

        void clear()
        {
            m_traits.clear();
            resetState();
            m_size=0;
        }

        common::ByteArray* appendMetaVar(size_t varTypeSize)
        {
            return m_traits.appendMetaVar(varTypeSize);
        }

        // end methods with traits

        int appendUint32(uint32_t val);

        template <typename T>
        int append(T* other);

        void appendBuffer(const common::ByteArrayShared& buf)
        {
            appendBuffer(common::SpanBuffer(buf));
        }

        void appendBuffer(common::ByteArrayShared&& buf)
        {
            appendBuffer(common::SpanBuffer(std::move(buf)));
        }

        void setCurrentOffset(size_t offs) noexcept
        {
            m_traits.m_offset=offs;
        }

        size_t currentOffset() const noexcept
        {
            return m_traits.m_offset;
        }

        void incCurrentOffset(int increment) noexcept
        {
            m_traits.m_offset+=increment;
        }

        void setSize(size_t size) noexcept
        {
            m_size=size;
        }

        size_t size() const noexcept
        {
            return m_size;
        }

        void incSize(int increment) noexcept
        {
            m_size+=increment;
        }

        AllocatorFactory* factory() const noexcept
        {
            return m_factory;
        }

        template <typename ContainerT>
        void copyToContainer(ContainerT& container) const
        {
            auto self=const_cast<decltype(this)>(this);

            self->resetState();
            container.reserve(container.size()+size());
            auto mContainer=mainContainer();
            if (mContainer && !mContainer->isEmpty())
            {
                container.append(*mContainer);
            }
            while (auto buf=nextBuffer())
            {
                container.append(buf.view());
            }
            self->resetState();
        }

        inline void setUseInlineBuffers(bool enable) noexcept
        {
            m_useInlineBuffers=enable;
        }
        inline bool isUseInlineBuffers() const noexcept
        {
            return m_useInlineBuffers;
        }

        inline bool isUseSharedBuffers() const noexcept
        {
            return m_useShareBuffers;
        }

    private:

        size_t m_size;
        AllocatorFactory* m_factory;

        bool m_useShareBuffers;
        bool m_useInlineBuffers;

        TraitsT m_traits;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREBUF_H
