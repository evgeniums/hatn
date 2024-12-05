/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/databuf.h
  *
  *      Definitions of various types
  *
  */

/****************************************************************************/

#ifndef HATNDATABUF_H
#define HATNDATABUF_H

#include <system_error>

#include <hatn/common/common.h>
#include <hatn/common/meta/consttraits.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Wrapper container of data buffers.
template <typename DataPointerT> struct DataBufWrapper
{
    template <typename ContainerT>
    DataBufWrapper(const ContainerT& container, size_t offset, size_t size) noexcept
        : m_buf(reinterpret_cast<DataPointerT>(container.data())+offset),m_size((size==0)?(container.size()-offset):size)
    {}

    template <typename ContainerT>
    DataBufWrapper(const ContainerT& container, size_t size) noexcept
        : m_buf(reinterpret_cast<DataPointerT>(container.data())),m_size(size)
    {}

    template <typename ContainerT>
    DataBufWrapper(const ContainerT& container) noexcept
        : m_buf(reinterpret_cast<DataPointerT>(container.data())),m_size(container.size())
    {}

    DataBufWrapper(DataPointerT buf, size_t size) noexcept
        : m_buf(buf),m_size(size)
    {}

    DataBufWrapper(typename ReverseConst<DataPointerT>::type buf, size_t size) noexcept
        : DataBufWrapper(const_cast<DataPointerT>(buf),size)
    {}

    DataBufWrapper() noexcept
        : m_buf(nullptr),m_size(0)
    {}

    DataBufWrapper(const char* str) noexcept
        : m_buf(str),m_size(strlen(str))
    {}

    DataBufWrapper(const DataBufWrapper& other) noexcept : m_buf(other.m_buf),m_size(other.m_size)
    {}
    DataBufWrapper& operator=(const DataBufWrapper& other) noexcept
    {
        if (&other!=this)
        {
            m_buf=other.m_buf;
            m_size=other.m_size;
        }
        return *this;
    }

    DataBufWrapper(DataBufWrapper&& other) noexcept : m_buf(other.m_buf),m_size(other.m_size)
    {
        other.reset();
    }
    DataBufWrapper& operator=(DataBufWrapper&& other) noexcept
    {
        if (&other!=this)
        {
            m_buf=other.m_buf;
            m_size=other.m_size;
            other.reset();
        }
        return *this;
    }

    ~DataBufWrapper()=default;

    inline size_t size() const noexcept
    {
        return m_size;
    }

    inline DataPointerT data() const noexcept
    {
        return m_buf;
    }

    inline bool isEmpty() const noexcept
    {
        return m_size==0;
    }

    inline bool isNull() const noexcept
    {
        return m_buf==nullptr;
    }

    inline void resize(size_t size)
    {
        if (size>m_size)
        {
            throw std::overflow_error("Size of DataBuf can not be increased");
        }
        m_size=size;
    }

    inline void set(char* buf, size_t size) noexcept
    {
        m_buf=const_cast<DataPointerT>(buf);
        m_size=size;
    }

    inline void set(const char* buf, size_t size) noexcept
    {
        m_buf=const_cast<DataPointerT>(buf);
        m_size=size;
    }

    template <typename ContainerT>
    void set(const ContainerT& container) noexcept
    {
        m_buf=reinterpret_cast<DataPointerT>(container.data());
        m_size=container.size();
    }

    inline void rebind(char* buf) noexcept
    {
        m_buf=const_cast<DataPointerT>(buf);
    }

    inline void rebind(const char* buf) noexcept
    {
        m_buf=const_cast<DataPointerT>(buf);
    }

    inline void reset() noexcept
    {
        m_buf=nullptr;
        m_size=0;
    }

    inline operator bool() const noexcept
    {
        return m_buf!=nullptr;
    }

    private:

        DataPointerT m_buf;
        size_t m_size;
};

using DataBuf=DataBufWrapper<char*>;
using ConstDataBuf=DataBufWrapper<const char*>;

HATN_COMMON_NAMESPACE_END

#endif // HATNDATABUF_H
