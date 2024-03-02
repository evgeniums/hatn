/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/spanbuffer.h
  *
  *      Utils for containers.
  *
  */

/****************************************************************************/

#ifndef HATNSPANBUFFER_H
#define HATNSPANBUFFER_H

#include <hatn/common/types.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/memorylockeddata.h>
#include <hatn/common/fixedbytearray.h>
#include <hatn/common/types.h>

HATN_COMMON_NAMESPACE_BEGIN

/**
 * @brief Span view of a data buffer
 */
class SpanBuffer
{
    private:

        struct Buffer
        {
            const char* ptr;
            size_t size;
        };

    public:

        SpanBuffer(const ByteArray& container, size_t spanOffset=0, size_t spanSize=0) noexcept
            : SpanBuffer(container.data(),container.size(),spanOffset,spanSize)
        {}
        SpanBuffer(const MemoryLockedArray& container, size_t spanOffset=0, size_t spanSize=0) noexcept
            : SpanBuffer(container.data(),container.size(),spanOffset,spanSize)
        {}
        SpanBuffer(const ConstDataBuf& container, size_t spanOffset=0, size_t spanSize=0) noexcept
            : SpanBuffer(container.data(),container.size(),spanOffset,spanSize)
        {}
        template <size_t Capacity, bool ThrowOnOverflow>
        SpanBuffer(const FixedByteArray<Capacity,ThrowOnOverflow>& container, size_t spanOffset=0, size_t spanSize=0) noexcept
            : SpanBuffer(container.data(),container.size(),spanOffset,spanSize)
        {}
        template <template <typename ...> class T>
        SpanBuffer(const T<char>& container, size_t spanOffset=0, size_t spanSize=0) noexcept
            : SpanBuffer(container.data(),container.size(),spanOffset,spanSize)
        {}

        SpanBuffer(const char* data, size_t size, size_t spanOffset=0, size_t spanSize=0) noexcept
            : m_buf(Buffer{data,size}),
              m_spanOffset(spanOffset),
              m_spanSize(spanSize)
        {}

        SpanBuffer():SpanBuffer(nullptr,0u,0u,0u)
        {}

#ifdef HATN_VARIANT_CPP17

        SpanBuffer(ByteArrayShared buf, size_t spanOffset=0, size_t spanSize=0) noexcept
            : m_buf(std::move(buf)),
              m_spanOffset(spanOffset),
              m_spanSize(spanSize)
        {}

        const char* bufData() const noexcept
        {
            try
            {
                if (!isSharedBuf())
                {
                    return lib::variantGet<Buffer>(m_buf).ptr;
                }
                const auto& buf=lib::variantGet<ByteArrayShared>(m_buf);
                if (buf.isNull())
                {
                    return nullptr;
                }
                return buf->data();
            }
            catch (...)
            {
            }
            return nullptr;
        }
        size_t bufSize() const noexcept
        {
            try
            {
                if (!isSharedBuf())
                {
                    return lib::variantGet<Buffer>(m_buf).size;
                }
                const auto& buf=lib::variantGet<ByteArrayShared>(m_buf);
                if (buf.isNull())
                {
                    return 0;
                }
                return buf->size();
            }
            catch (...)
            {
            }
            return 0;
        }

        ByteArrayShared* sharedBuf() const
        {
            try
            {
                if (isSharedBuf())
                {
                    return &(lib::variantGet<ByteArrayShared>(const_cast<SpanBuffer*>(this)->m_buf));
                }
            }
            catch (...)
            {
            }
            return nullptr;
        }

        bool isSharedBuf() const noexcept
        {
            return m_buf.index()==1;
        }

        void setData(const char* ptr)
        {
            if (isSharedBuf())
            {
                Assert(false,"Can not set data pointer for shared buffer");
            }

            lib::variantGet<Buffer>(m_buf).ptr=ptr;
        }

        void setSize(size_t size)
        {
            if (isSharedBuf())
            {
                Assert(false,"Can not set size for shared buffer");
            }

            lib::variantGet<Buffer>(m_buf).size=size;
        }
#else

        SpanBuffer(ByteArrayShared buf, size_t spanOffset=0, size_t spanSize=0) noexcept
            : m_barr(std::move(buf)),
              m_buf(Buffer{nullptr,0}),
              m_spanOffset(spanOffset),
              m_spanSize(spanSize)
        {}

        const char* bufData() const noexcept
        {
            return m_barr.isNull()?m_buf.ptr:m_barr->data();
        }

        size_t bufSize() const noexcept
        {
            return m_barr.isNull()?m_buf.size:m_barr->size();
        }

        ByteArrayShared* sharedBuf() const
        {
            if (!m_barr.isNull())
            {
                return &(const_cast<SpanBuffer*>(this)->m_barr);
            }
            return nullptr;
        }

        bool isSharedBuf() const noexcept
        {
            return !m_barr.isNull();
        }

        void setData(const char* ptr)
        {
            if (isSharedBuf())
            {
                Assert(false,"Can not set data pointer for shared buffer");
            }

            m_buf.ptr=ptr;
        }

        void setSize(size_t size)
        {
            if (isSharedBuf())
            {
                Assert(false,"Can not set size for shared buffer");
            }

            m_buf.size=size;
        }
#endif
        size_t spanOffset() const noexcept
        {
            return m_spanOffset;
        }
        size_t spanSize() const noexcept
        {
            return m_spanSize;
        }
        void setSpanOffset(size_t offset) noexcept
        {
            m_spanOffset=offset;
        }
        void setSpanSize(size_t size) noexcept
        {
            m_spanSize=size;
        }

        bool isEmpty() const noexcept
        {
            return bufSize()==0;
        }

        bool empty() const noexcept
        {
            return isEmpty();
        }

        operator bool() const noexcept
        {
            return !isEmpty();
        }

        static std::pair<bool,ConstDataBuf> span(const char* bufData, size_t bufSize, size_t spanOffset, size_t spanSize) noexcept
        {
            return std::make_pair<bool,ConstDataBuf>(bufSize>=(spanOffset+spanSize),
                                                     ConstDataBuf(bufData+spanOffset,
                                                                  (spanSize==0)?(bufSize-spanOffset):spanSize
                                                                 )
                                                     );
        }

        template <typename ContainerT>
        static std::pair<bool,ConstDataBuf> span(const ContainerT& container, size_t offset, size_t size) noexcept
        {
            return span(container.data(),container.size(),offset,size);
        }

        std::pair<bool,ConstDataBuf> span() const noexcept
        {
            return span(bufData(),bufSize(),m_spanOffset,m_spanSize);
        }

        ConstDataBuf view() const
        {
            auto p=span(bufData(),bufSize(),m_spanOffset,m_spanSize);
            if (!p.first)
            {
                throw ErrorException(Error(CommonError::INVALID_SIZE));
            }
            return p.second;
        }

        static void append(pmr::vector<SpanBuffer>& buffersA, pmr::vector<SpanBuffer> buffersB)
        {
            buffersA.insert(buffersA.end(),std::make_move_iterator(buffersB.begin()),std::make_move_iterator(buffersB.end()));
        }
        static void append(pmr::vector<SpanBuffer>& buffersA, SpanBuffer bufferB)
        {
            buffersA.push_back(std::move(bufferB));
        }

    private:

#ifdef HATN_VARIANT_CPP17
        lib::variant<Buffer,ByteArrayShared> m_buf;
#else
        ByteArrayShared m_barr;
        Buffer m_buf;
#endif
        size_t m_spanOffset;
        size_t m_spanSize;
};
using SpanBuffers=pmr::vector<SpanBuffer>;

struct SpanBufferTraits
{
    static size_t size(const SpanBuffer& buffer)
    {
        return buffer.view().size();
    }

    static size_t size(const SpanBuffers& buffers)
    {
        size_t sum=0;
        for (auto&& it:buffers)
        {
            sum+=size(it);
        }
        return sum;
    }

    static SpanBuffer extractPrefix(const SpanBuffer& buffer, DataBuf& prefix, size_t prefixSize)
    {
        auto view=buffer.view();
        if (view.size()<prefixSize)
        {
            throw ErrorException(Error(CommonError::INVALID_SIZE));
        }
        prefix.set(view.data(),prefixSize);
        return SpanBuffer(view,prefixSize);
    }

    static std::pair<SpanBuffer,SpanBuffer> split(const SpanBuffer& buffer, size_t leftSize)
    {
        auto view=buffer.view();
        if (view.size()<leftSize)
        {
            throw ErrorException(Error(CommonError::INVALID_SIZE));
        }
        if (view.size()==leftSize)
        {
            return std::make_pair(buffer,SpanBuffer());
        }
        return std::make_pair(SpanBuffer(view,0,leftSize),SpanBuffer(view,leftSize,view.size()-leftSize));
    }

    static SpanBuffers extractPrefix(const SpanBuffers& buffers, DataBuf& prefix, size_t prefixSize)
    {
        if (buffers.empty())
        {
            throw ErrorException(Error(CommonError::INVALID_SIZE));
        }

        auto view=buffers.front().view();
        if (view.size()<prefixSize)
        {
            throw ErrorException(Error(CommonError::INVALID_SIZE));
        }

        prefix.set(view.data(),prefixSize);

        if (view.size()==prefixSize)
        {
            if (buffers.size()==1)
            {
                return SpanBuffers();
            }
            SpanBuffers result;
            result.reserve(buffers.size()-1);
            for (size_t i=1;i<buffers.size();i++)
            {
                result.push_back(buffers[i]);
            }
        }
        SpanBuffers result=buffers;
        result[0]=SpanBuffer(view,prefixSize);
        return result;
    }

    static std::pair<SpanBuffers,SpanBuffers> split(const SpanBuffers& buffers, size_t leftSize)
    {
        SpanBuffers left;
        SpanBuffers right;
        size_t sum=0;
        for (auto&& it:buffers)
        {
            auto view=it.view();
            auto viewSize=view.size();
            if (sum>=leftSize)
            {
                right.push_back(it);
                sum+=viewSize;
            }
            else
            {
                sum+=viewSize;
                if (sum>leftSize)
                {
                    auto diff=sum-leftSize;
                    SpanBuffer leftBuf(view,0,diff);
                    left.push_back(leftBuf);
                    SpanBuffer rightBuf(view,diff,viewSize-diff);
                    left.push_back(rightBuf);
                }
                else
                {
                    left.push_back(it);
                }
            }
        }
        return std::make_pair(std::move(left),std::move(right));
    }
};

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNSPANBUFFER_H
