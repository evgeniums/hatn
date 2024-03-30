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
#include <hatn/common/objecttraits.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/allocatorfactory.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

class WireBufSolid;

class WireBufBase
{
    public:

        explicit WireBufBase(
            AllocatorFactory* factory,
            bool useShareBuffers=false
        ) : WireBufBase(0,factory,useShareBuffers)
        {}

        explicit WireBufBase(
            size_t size=0,
            AllocatorFactory* factory=nullptr,
            bool useShareBuffers=false
        ) noexcept :
            m_size(size),
            m_factory(factory),
            m_useShareBuffers(useShareBuffers),
            m_useInlineBuffers(false)
        {}

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

    protected:

        void setFactory(AllocatorFactory* factory) noexcept
        {
            m_factory=factory;
        }

        size_t m_size;
        AllocatorFactory* m_factory;

        bool m_useShareBuffers;
        bool m_useInlineBuffers;
};

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

    size_t m_offset=0;
};

template <typename TraitsT>
class WireBuf : public common::WithTraits<TraitsT>,
                public WireBufBase
{    
    public:

        constexpr static bool isSingleBuffer() noexcept
        {
            return TraitsT::isSingleBuffer();
        }

        explicit WireBuf(
            TraitsT&& traits,
            AllocatorFactory* factory,
            bool useShareBuffers=false
        ) :  WireBuf(std::move(traits),0,factory,useShareBuffers)
        {}

        explicit WireBuf(
            TraitsT&& traits,
            size_t size=0,
            AllocatorFactory* factory=nullptr,
            bool useShareBuffers=false
        ) noexcept
            : common::WithTraits<TraitsT>(std::move(traits)),
              WireBufBase(size,factory,useShareBuffers)
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

        common::SpanBuffer nextBuffer() const noexcept
        {
            return this->traits().nextBuffer();
        }

        common::ByteArray* mainContainer() const noexcept
        {
            return this->traits().mainContainer();
        }

        void appendBuffer(common::SpanBuffer&& buf)
        {
            this->traits().appendBuffer(std::move(buf));
        }

        void appendBuffer(const common::SpanBuffer& buf)
        {
            this->traits().appendBuffer(buf);
        }

        void setCurrentMainContainer(common::ByteArray* currentMainContainer) noexcept
        {
            this->traits().setCurrentMainContainer(currentMainContainer);
        }

        void reserveMetaCapacity(size_t size)
        {
            this->traits().reserveMetaCapacity(size);
        }

        void resetState()
        {
            this->traits().m_offset=0;
            this->traits().resetState();
        }

        void clear()
        {
            this->traits().clear();
            resetState();
            m_size=0;
        }

        common::ByteArray* appendMetaVar(size_t varTypeSize)
        {
            return this->traits().appendMetaVar(varTypeSize);
        }

        void setCurrentOffset(size_t offs) noexcept
        {
            this->traits().m_offset=offs;
        }

        size_t currentOffset() const noexcept
        {
            return this->traits().m_offset;
        }

        void incCurrentOffset(int increment) noexcept
        {
            this->traits().m_offset+=increment;
        }

        template <typename ContainerT>
        void copyToContainer(ContainerT& container) const
        {
            auto* self=const_cast<std::remove_const_t<std::remove_pointer_t<decltype(this)>>*>(this);

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

        WireBufSolid toSolidWireBuf() const;

        //! @todo remove it
        WireBufSolid toSingleWireData() const;

        // end methods with traits                   
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREBUF_H
