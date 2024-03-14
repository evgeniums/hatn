/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/wiredata.сpp
  *
  *      Packed (serialized) DataUnit for wire.
  *
  */

#include <hatn/dataunit/stream.h>

#include <hatn/common/pmr/withstaticallocator.h>
#include <hatn/common/pmr/withstaticallocator.ipp>

#define HATN_WIREDATA_SRC
#define HATN_WITH_STATIC_ALLOCATOR_SRC
#include <hatn/dataunit/wiredata.h>
#include <hatn/dataunit/wiredatapack.h>
#undef HATN_WITH_STATIC_ALLOCATOR_SRC

HATN_DATAUNIT_NAMESPACE_BEGIN

/********************** WireData **************************/

//---------------------------------------------------------------
WireDataSingle WireData::toSingleWireData() const
{
    auto f=factory();
    common::pmr::memory_resource* memResource=f?f->dataMemoryResource():common::pmr::get_default_resource();
    common::ByteArray singleBuf(memResource);
    copyToContainer(singleBuf);
    return WireDataSingle(std::move(singleBuf),f);
}

//---------------------------------------------------------------
int WireData::appendUint32(uint32_t val)
{
    auto* buf=mainContainer();
    if (!isSingleBuffer())
    {
        buf=appendMetaVar(sizeof(uint32_t));
    }
    auto consumed=Stream<uint32_t>::packVarInt(buf,val);
    incSize(consumed);
    if (!isSingleBuffer())
    {
        setActualMetaVarSize(consumed);
    }
    return consumed;
}

//---------------------------------------------------------------
int WireData::append(WireData* other)
{
    other->resetState();

    int size=static_cast<int>(other->size());
    if (isSingleBuffer())
    {
        // copy pre-serialized data to signle data container
        auto* targetBuf=mainContainer();
        other->copyToContainer(*targetBuf);
    }
    else
    {
        auto srcContainer=other->mainContainer();
        if (srcContainer && !srcContainer->isEmpty())
        {
            // allocated new shared buffer and copy prepared main container buffer to it
            auto&& sharedBuf=factory()->createObject<::hatn::common::ByteArrayManaged>(
                                srcContainer->data(),srcContainer->size(),factory()->dataMemoryResource()
                            );
            appendBuffer(std::move(sharedBuf));
        }

        if (!other->isSingleBuffer())
        {
            // copy chain of shared buffers
            while (auto buf=other->nextBuffer())
            {
                appendBuffer(std::move(buf));
            }
        }
    }

    other->resetState();
    incSize(size);

    return size;
}

/********************** WireDataSingle **************************/


/********************** WireDataSingleShared **************************/

/********************** WireDataChained **************************/

//---------------------------------------------------------------
WireDataChained::WireDataChained(
        AllocatorFactory* factory
    ) : WireDataSingleShared(factory),
        m_meta(factory->dataMemoryResource()),
        m_currentMainContainer(nullptr),
        m_bufferChain(factory->objectAllocator<common::ByteArrayShared>()),
        m_cursor(m_bufferChain.end())
{}

//---------------------------------------------------------------
common::ByteArray* WireDataChained::appendMetaVar(size_t varTypeSize)
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

//---------------------------------------------------------------
void WireDataChained::setActualMetaVarSize(size_t actualVarSize)
{
    auto& buf=m_bufferChain.back();
    buf.setSpanSize(actualVarSize);
}

//---------------------------------------------------------------
void WireDataChained::doClear()
{
    m_bufferChain.clear();
    m_meta.reset();
    m_tmpInline.reset();
    m_cursor=m_bufferChain.end();
}

//---------------------------------------------------------------
void WireDataChained::resetState()
{
    if (!m_bufferChain.empty())
    {
        m_cursor=m_bufferChain.begin();
    }
    else
    {
        m_cursor=m_bufferChain.end();
    }
    WireData::resetState();
}

//---------------------------------------------------------------
void WireDataChained::appendBuffer(const common::SpanBuffer& buf)
{
    m_bufferChain.push_back(buf);
    m_cursor=m_bufferChain.begin();
}

//---------------------------------------------------------------
void WireDataChained::appendBuffer(common::SpanBuffer&& buf)
{
    m_bufferChain.push_back(std::move(buf));
    m_cursor=m_bufferChain.begin();
}

//---------------------------------------------------------------
common::SpanBuffer WireDataChained::nextBuffer() const noexcept
{
    if (m_cursor!=m_bufferChain.end())
    {
        return *m_cursor++;
    }
    return common::SpanBuffer();
}

//---------------------------------------------------------------
void WireDataChained::reserveMetaCapacity(size_t capacity)
{
    m_meta.reserve(capacity);
}

/********************** WireDataPack **************************/

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END
