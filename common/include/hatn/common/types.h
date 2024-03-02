/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/types.h
  *
  *      Definitions of various types
  *
  */

/****************************************************************************/

#ifndef HATNTYPES_H
#define HATNTYPES_H

#include <memory>

#if __cplusplus < 201703L
    #include <boost/utility/string_view.hpp>
#endif

#if __cplusplus < 201703L || (defined (IOS_SDK_VERSION_X10) && IOS_SDK_VERSION_X10<120)
    #include <boost/variant.hpp>
    #include <boost/optional.hpp>
#else
    #include <variant>
    #include <optional>
#endif

#include <fmt/core.h>

#include <hatn/common/common.h>
#include <hatn/common/metautils.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace lib
{

#if __cplusplus < 201703L
    using string_view=boost::string_view;
#else
    using string_view=std::string_view;
#endif
template <typename T> string_view toStringView(const T& buf) noexcept
{
    return string_view(buf.data(),buf.size());
}

#if __cplusplus < 201703L || (defined (IOS_SDK_VERSION_X10) && IOS_SDK_VERSION_X10<120)
    template <typename ...Types> using variant=boost::variant<Types...>;
    template <typename T,typename ...Types> constexpr T& variantGet(variant<Types...>& var) noexcept
    {
        return boost::get<T>(var);
    }
    template <typename T,typename ...Types> constexpr const T& variantGet(const variant<Types...>& var) noexcept
    {
        return boost::get<T>(var);
    }
    template <typename T> using optional=boost::optional<T>;
#else
    template <typename ...Types> using variant=std::variant<Types...>;
    template <typename T,typename ...Types> constexpr T& variantGet(variant<Types...>& var) noexcept
    {
        return std::get<T>(var);
    }
    template <typename T,typename ...Types> constexpr const T& variantGet(const variant<Types...>& var) noexcept
    {
        return std::get<T>(var);
    }
    template <typename T> using optional=std::optional<T>;
    #define HATN_VARIANT_CPP17
#endif

template <typename T> inline void destroyAt(T* obj) noexcept
{
#if __cplusplus < 201703L
    obj->~T();
#else
    std::destroy_at(obj);
#endif
}

}

/**
 * @brief The KeyRange struct to be used as a key in containers of ranges
 */
struct KeyRange
{
    size_t from=0;
    size_t to=0;

    friend inline bool operator ==(const KeyRange& left,const KeyRange& right) noexcept
    {
        return (left.from>=right.from&&left.to<=right.to)
                ||
               (right.from>=left.from&&right.to<=left.to)
              ;
    }
    friend inline bool operator <(const KeyRange& left,const KeyRange& right) noexcept
    {
        return left.to<right.from;
    }
};

#if __cplusplus >= 201703L
    #define HATN_FALLTHROUGH [[fallthrough]];
#else
    #define HATN_FALLTHROUGH
#endif

template <typename T, T defaultValue> struct ValueOrDefault
{
    T val;
    ValueOrDefault():val(defaultValue)
    {}
};

template <typename T> struct PointerWithInit
{
    T* ptr;
    PointerWithInit(T* ptr=nullptr) noexcept : ptr(ptr)
    {}

    bool isNull() const noexcept
    {
        return ptr==nullptr;
    }

    inline T* operator ->() noexcept
    {
        return ptr;
    }
    inline T* operator ->() const noexcept
    {
        return ptr;
    }
};

template <typename T, const T* defaultVal=nullptr> struct ConstPointerWithInit
{
    const T* ptr;
    ConstPointerWithInit(const T* ptr=defaultVal) noexcept : ptr(ptr)
    {}

    bool isNull() const noexcept
    {
        return ptr==nullptr;
    }

    inline const T* operator ->() noexcept
    {
        return ptr;
    }
    inline const T* operator ->() const noexcept
    {
        return ptr;
    }
};

//! Wrapper container of data buffers
template <typename DataPointerT> struct DataBufWrapper
{
    template <typename ContainerT>
    DataBufWrapper(const ContainerT& container, size_t offset, size_t size=0) noexcept
        : m_buf(reinterpret_cast<DataPointerT>(container.data())+offset),m_size((size==0)?(container.size()-offset):size)
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

    inline void reset() noexcept
    {
        m_buf=nullptr;
        m_size=0;
    }

    private:

        DataPointerT m_buf;
        size_t m_size;
};

using DataBuf=DataBufWrapper<char*>;
using ConstDataBuf=DataBufWrapper<const char*>;

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNTYPES_H
