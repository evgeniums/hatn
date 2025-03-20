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
#include <hatn/common/databuf.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/allocatorfactory.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

class WireBufSolid;

class HATN_DATAUNIT_EXPORT WireBufBase
{
    public:

        explicit WireBufBase(
            const AllocatorFactory* factory,
            bool useShareBuffers=false
        ) : WireBufBase(0,factory,useShareBuffers)
        {}

        explicit WireBufBase(
            size_t size=0,
            const AllocatorFactory* factory=nullptr,
            bool useShareBuffers=false
        ) noexcept :
            m_size(size),
            m_factory(factory),
            m_useShareBuffers(useShareBuffers),
            m_useInlineBuffers(false)
        {}

        inline void setSize(size_t size) noexcept
        {
            m_size=size;
        }

        inline size_t size() const noexcept
        {
            return m_size;
        }

        inline void incSize(int increment) noexcept
        {
            m_size+=increment;
        }

        inline const AllocatorFactory* factory() const noexcept
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

        inline void setUseSharedBuffers(bool enable) noexcept
        {
            m_useShareBuffers=enable;
        }

    protected:

        inline void setFactory(const AllocatorFactory* factory) noexcept
        {
            m_factory=factory;
        }

        size_t m_size;
        const AllocatorFactory* m_factory;

        bool m_useShareBuffers;
        bool m_useInlineBuffers;
};

struct HATN_DATAUNIT_EXPORT WireBufChainItem
{
    WireBufChainItem(
        common::DataBuf buf,
        bool inMeta=true
        ) : buf(buf),inMeta(inMeta)
    {}

    common::DataBuf buf;
    bool inMeta;
};

struct HATN_DATAUNIT_EXPORT WireBufTraits
{
    void resetState() {m_offset=0;}
    void setCurrentMainContainer(common::ByteArray* /*currentMainContainer*/) noexcept
    {}

    common::SpanBuffers buffers() const
    {
        Assert(false,"Invalid operation");
        return common::SpanBuffers{};
    }

    std::vector<WireBufChainItem> chain() const
    {
        Assert(false,"Invalid operation");
        return std::vector<WireBufChainItem>{};
    }

    common::SpanBuffers chainBuffers(const AllocatorFactory* =AllocatorFactory::getDefault()) const
    {
        Assert(false,"Invalid operation");
        return common::SpanBuffers{};
    }

    const common::ByteArray* meta() const
    {
        Assert(false,"Invalid operation");
        return nullptr;
    }

    common::DataBuf* appendMetaVar(size_t varTypeSize)
    {
        Assert(false,"Invalid operation");
        std::ignore=varTypeSize;
        return nullptr;
    }

    void clear(){}

    mutable size_t m_offset=0;
};

template <typename BufferT, typename ContainerT>
void copyToContainer(const BufferT& buf, ContainerT* container)
{
    auto& src=const_cast<BufferT&>(buf);

    src.resetState();
    while (auto buf=src.nextBuffer())
    {
        container->append(buf);
    }
    src.resetState();
}

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
            const AllocatorFactory* factory,
            bool useShareBuffers=false
        ) :  WireBuf(std::move(traits),0,factory,useShareBuffers)
        {}

        explicit WireBuf(
            TraitsT&& traits,
            size_t size=0,
            const AllocatorFactory* factory=nullptr,
            bool useShareBuffers=false
        ) noexcept
            : common::WithTraits<TraitsT>(std::move(traits)),
              WireBufBase(size,factory,useShareBuffers)
        {}

        // begin methods with traits

        /**
         * @brief Append uint32 value.
         * @param val Vaue.
         * @return Increment of size.
         *
         * Size of WireBuf is automatically increased.
         */
        int appendUint32(uint32_t val);

        /**
         * @brief Append other WireBuf
         * @param other Other WireBuf.
         * @return Increment of size.
         *
         * Size of WireBuf is automatically increased.
         */
        template <typename T>
        int append(const T& other)
        {
            auto size=this->traits().append(other,this->factory());
            this->incSize(size);
            return size;
        }

        /**
         * @brief Push shared buffer to WireBuf.
         * @param buf Buffer.
         *
         * Size of WireBuf is NOT automatically increased.
         */
        void appendBuffer(const common::ByteArrayShared& buf)
        {
            appendBuffer(common::SpanBuffer(buf));
        }

        /**
         * @brief Push shared buffer to WireBuf.
         * @param buf Buffer.
         *
         * Size of WireBuf is NOT automatically increased.
         */
        void appendBuffer(common::ByteArrayShared&& buf)
        {
            appendBuffer(common::SpanBuffer(std::move(buf)));
        }

        /**
         * @brief Push span buffer to WireBuf.
         * @param buf Buffer.
         *
         * Size of WireBuf is NOT automatically increased.
         */
        void appendBuffer(common::SpanBuffer&& buf)
        {
            this->traits().appendBuffer(std::move(buf));
        }

        /**
         * @brief Push span buffer to WireBuf.
         * @param buf Buffer.
         *
         * Size of WireBuf is NOT automatically increased.
         */
        void appendBuffer(const common::SpanBuffer& buf)
        {
            this->traits().appendBuffer(buf);
        }

        /**
         * @brief Push data buffer to WireBuf.
         * @param buf Buffer.
         *
         * Size of WireBuf is NOT automatically increased.
         */
        void appendBuffer(common::DataBuf buf)
        {
            this->traits().appendBuffer(std::move(buf));
        }

        common::DataBuf nextBuffer() const noexcept
        {
            return this->traits().nextBuffer();
        }

        common::ByteArray* mainContainer() const noexcept
        {
            return this->traits().mainContainer();
        }

        common::ByteArrayShared sharedMainContainer() const noexcept
        {
            return this->traits().sharedMainContainer();
        }

        void setCurrentMainContainer(common::ByteArray* currentMainContainer) noexcept
        {
            this->traits().setCurrentMainContainer(currentMainContainer);
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

        common::DataBuf* appendMetaVar(size_t varTypeSize)
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

        common::SpanBuffers buffers() const
        {
            return this->traits().buffers();
        }

        std::vector<WireBufChainItem> chain() const
        {
            return this->traits().chain();
        }

        common::SpanBuffers chainBuffers(const AllocatorFactory* factory=AllocatorFactory::getDefault()) const
        {
            return this->traits().chainBuffers(factory);
        }

        const common::ByteArray* meta() const
        {
            return this->traits().meta();
        }

        WireBufSolid toSolidWireBuf() const;

        // end methods with traits                   
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREBUF_H
