/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/wiredata.h
  *
  *   Polymorphic variant of wire buffers.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITWIREDATA_H
#define HATNDATAUNITWIREDATA_H

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/wirebuf.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/wirebufchained.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

template <typename ImplT> class WireDataDerived;

class HATN_DATAUNIT_EXPORT WireData
{
public:

    WireData(
        WireBufBase &impl
    ) : baseImpl(impl)
    {}

    virtual ~WireData();
    WireData(const WireData&)=default;
    // WireData& operator=(const WireData&)=default;
    WireData(WireData&&)=default;
    // WireData& operator=(WireData&&)=default;

    virtual common::DataBuf nextBuffer() const noexcept=0;
    virtual common::ByteArray* mainContainer() const noexcept=0;
    virtual void appendBuffer(common::SpanBuffer&& buf)=0;
    virtual void appendBuffer(const common::SpanBuffer& buf)=0;
    virtual void appendBuffer(common::DataBuf buf)=0;
    virtual bool isSingleBuffer() const noexcept=0;
    virtual void resetState()=0;
    virtual void setCurrentMainContainer(common::ByteArray* currentMainContainer) noexcept=0;
    virtual common::DataBuf* appendMetaVar(size_t varTypeSize)=0;
    virtual void setCurrentOffset(size_t offs) noexcept=0;
    virtual size_t currentOffset() const noexcept=0;
    virtual void incCurrentOffset(int increment) noexcept=0;
    virtual int appendUint32(uint32_t val)=0;
    virtual int append(const WireData& other)=0;
    virtual WireBufSolid toSolidWireBuf() const=0;
    virtual void clear()=0;
    virtual common::SpanBuffers buffers() const=0;
    virtual std::vector<WireBufChainItem> chain() const=0;
    virtual common::SpanBuffers chainBuffers(const AllocatorFactory* factory=AllocatorFactory::getDefault()) const=0;
    virtual const common::ByteArray* meta() const=0;

    inline void appendBuffer(const common::ByteArrayShared& buf)
    {
        appendBuffer(common::SpanBuffer(buf));
    }
    inline void appendBuffer(common::ByteArrayShared&& buf)
    {
        appendBuffer(common::SpanBuffer(std::move(buf)));
    }

    inline bool isUseSharedBuffers() const noexcept
    {
        return baseImpl.isUseSharedBuffers();
    }

    //! Set wired size
    inline void setSize(size_t size) noexcept
    {
        baseImpl.setSize(size);
    }

    //! Get wired size
    inline size_t size() const noexcept
    {
        return baseImpl.size();
    }

    //! Increment size
    inline void incSize(int increment) noexcept
    {
        baseImpl.incSize(increment);
    }

    inline const AllocatorFactory* factory() const noexcept
    {
        return baseImpl.factory();
    }

    inline void setUseInlineBuffers(bool enable) noexcept
    {
        baseImpl.setUseInlineBuffers(enable);
    }
    inline bool isUseInlineBuffers() const noexcept
    {
        return baseImpl.isUseInlineBuffers();
    }

private:

    WireBufBase& baseImpl;
};

template <typename ImplT>
class WireDataDerived : public common::WithImpl<ImplT>, public WireData
{
public:

    template <typename ...Args>
    WireDataDerived(Args&&... args)
        : common::WithImpl<ImplT>(std::forward<Args>(args)...),
          WireData(this->impl())
    {}

    virtual common::DataBuf nextBuffer() const noexcept override
    {
        return this->impl().nextBuffer();
    }

    virtual common::ByteArray* mainContainer() const noexcept override
    {
        return this->impl().mainContainer();
    }

    using WireData::appendBuffer;

    virtual void appendBuffer(common::SpanBuffer&& buf) override
    {
        this->impl().appendBuffer(std::move(buf));
    }

    virtual void appendBuffer(const common::SpanBuffer& buf) override
    {
        this->impl().appendBuffer(buf);
    }

    virtual void appendBuffer(common::DataBuf buf) override
    {
        this->impl().appendBuffer(std::move(buf));
    }

    virtual bool isSingleBuffer() const noexcept override
    {
        return this->impl().isSingleBuffer();
    }

    virtual void resetState() override
    {
        this->impl().resetState();
    }

    virtual void setCurrentMainContainer(common::ByteArray* container) noexcept override
    {
        this->impl().setCurrentMainContainer(container);
    }

    virtual common::DataBuf* appendMetaVar(size_t varTypeSize) override
    {
        return this->impl().appendMetaVar(varTypeSize);
    }

    virtual void setCurrentOffset(size_t offs) noexcept override
    {
        this->impl().setCurrentOffset(offs);
    }

    virtual size_t currentOffset() const noexcept override
    {
        return this->impl().currentOffset();
    }

    virtual void incCurrentOffset(int increment) noexcept override
    {
        this->impl().incCurrentOffset(increment);
    }

    virtual int appendUint32(uint32_t val) override
    {
        return this->impl().appendUint32(val);
    }

    virtual int append(const WireData& other) override
    {
        return this->impl().append(other);
    }

    virtual void clear() override
    {
        this->impl().clear();
    }

    virtual common::SpanBuffers buffers() const override
    {
        return this->impl().buffers();
    }

    virtual std::vector<WireBufChainItem> chain() const override
    {
        return this->impl().chain();
    }

    virtual common::SpanBuffers chainBuffers(const AllocatorFactory* factory=AllocatorFactory::getDefault()) const override
    {
        return this->impl().chainBuffers(factory);
    }

    virtual const common::ByteArray* meta() const override
    {
        return this->impl().meta();
    }

    WireBufSolid toSolidWireBuf() const override
    {
        return this->impl().toSolidWireBuf();
    }
};

using WireDataSingle=WireDataDerived<WireBufSolid>;
using WireDataSingleShared=WireDataDerived<WireBufSolidShared>;
using WireDataChained=WireDataDerived<WireBufChained>;

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREDATA_H
