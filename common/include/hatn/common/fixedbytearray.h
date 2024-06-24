/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/fixedbytearray.h
  *
  *     Byte array of fixed capacity.
  *
  */

/****************************************************************************/

#ifndef HATNFIXEDBYTEARRAY_H
#define HATNFIXEDBYTEARRAY_H

#include <cstring>
#include <array>
#include <vector>

#include <hatn/common/common.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/managedobject.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/meta/cstrlength.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace detail
{

template <typename T, bool ThrowOnOverflow>
struct FixedByteArrayTraits
{
};

template <typename T>
struct FixedByteArrayTraits<T,true>
{
    inline static bool checkSize(size_t& size)
    {
        if (size>T::Capacity)
        {
            size=T::Capacity;
            throw std::overflow_error("Data size exceeds fixed byte array capacity!");
        }
        return true;
    }
};

template <typename T>
struct FixedByteArrayTraits<T,false>
{
    constexpr inline static bool checkSize(size_t& size) noexcept
    {
        if (size>T::Capacity)
        {
            size=T::Capacity;
            return false;
        }
        return true;
    }
};

}

/**
 * @brief Byte array with fixed capacity
 */
template <size_t CapacityS, bool ThrowOnOverflow=false>
class FixedByteArray
{
    public:

        using value_type=char;
        using type=FixedByteArray<CapacityS,ThrowOnOverflow>;
        constexpr static const size_t Capacity=CapacityS;

        //! Default ctor
        FixedByteArray() noexcept
            : m_size(0),
              m_externalDataPtr(nullptr)
        {}

        //! Copy constuctor
        FixedByteArray(const FixedByteArray& other) noexcept
            : FixedByteArray()
        {
            other.isRawBuffer()?loadInline(other.data(),other.size()):load(other.data(),other.size());
        }

        //! Move constructor
        FixedByteArray(
                FixedByteArray&& other
            ) noexcept : FixedByteArray()
        {
            other.isRawBuffer()?loadInline(other.data(),other.size()):load(other.data(),other.size());
            other.m_size=0;
            other.m_externalDataPtr=nullptr;
        }

        //! Ctor from string
        FixedByteArray(const std::string& other)
            : FixedByteArray()
        {
            load(other.data(),other.size());
        }

        //! Ctor from std::vector
        FixedByteArray(const std::vector<char>& other)
            : FixedByteArray()
        {
            load(other.data(),other.size());
        }

        //! Ctor from ByteArray
        FixedByteArray(const ByteArray& other)
            : FixedByteArray()
        {
            load(other.data(),other.size());
        }

        //! Ctor from data buffer
        FixedByteArray(
            const char* data, //!< Data buffer
            size_t size //!< Data size
        ) : FixedByteArray()
        {
            load(data,size);
        }

        //! Ctor from data buffer
        FixedByteArray(
            const char* data, //!< Data buffer
            size_t size, //!< Data size
            bool inlineRawBuffer
        ) : FixedByteArray()
        {
            if (inlineRawBuffer)
            {
                loadInline(data,size);
            }
            else
            {
                load(data,size);
            }
        }

        //! Ctor from null-terminated string
        FixedByteArray(
            const char* data //!< String
        ) : FixedByteArray()
        {
            append(data);
        }

        //! Copy constuctor from FixedByteArray of other capacity
        template <size_t capacity=Capacity, bool throwOverflow=ThrowOnOverflow>
        explicit FixedByteArray(const FixedByteArray<capacity,throwOverflow>& other)
            : FixedByteArray()
        {
            other.isRawBuffer()?loadInline(other.data(),other.size()):load(other.data(),other.size());
        }

        //! Move constructor from FixedByteArray of other capacity
        template <size_t capacity=Capacity, bool throwOverflow=ThrowOnOverflow>
        FixedByteArray(
                FixedByteArray<capacity,throwOverflow>&& other
            ) : FixedByteArray()
        {
            other.isRawBuffer()?loadInline(other.data(),other.size()):load(other.data(),other.size());
            other.m_size=0;
            other.m_externalDataPtr=nullptr;
        }

        //! Dtor
        virtual ~FixedByteArray()=default;

        //! Move assignment operator
        template <size_t CapacityY, bool ThrowOnOverflowY>
        FixedByteArray& operator=(FixedByteArray<CapacityY,ThrowOnOverflowY>&& other)
        {
            other.isRawBuffer()?loadInline(other.data(),other.size()):load(other.data(),other.size());
            other.m_size=0;
            other.m_externalDataPtr=nullptr;
            return *this;
        }

        //! Move assignment operator
        inline FixedByteArray& operator=(FixedByteArray&& other) noexcept
        {
            if (this!=&other)
            {
                other.isRawBuffer()?loadInline(other.data(),other.size()):load(other.data(),other.size());
                other.m_size=0;
                other.m_externalDataPtr=nullptr;
            }
            return *this;
        }

        //! Assignment operator
        inline FixedByteArray& operator=(const FixedByteArray& other) noexcept
        {
            if (&other!=this)
            {
                other.isRawBuffer()?loadInline(other.data(),other.size()):load(other.data(),other.size());
            }
            return *this;
        }

        //! Assignment to container
        template <typename ContainerT> FixedByteArray& operator=(const ContainerT& container)
        {
            load(container.data(),container.size());
            return *this;
        }

        //! Assignment to c-string
        FixedByteArray& operator=(const char* str)
        {
            load(str);
            return *this;
        }

        //! Default value STR_ID_TYPE
        inline static const FixedByteArray& defaultUnknown() noexcept
        {
            static FixedByteArray v("unknown");
            return v;
        }

        //! Get const reference to self
        inline const FixedByteArray& value() const noexcept
        {
            return *this;
        }

        //! Get data buffer
        inline char* data() const noexcept
        {
            return privData();
        }

        //! Get data buffer
        inline char* data() noexcept
        {
            return privData();
        }

        //! Const data buffer
        inline const char* constData() const noexcept
        {
            return privDataConst();
        }

        //! Array size
        inline size_t size() const noexcept
        {
            return m_size;
        }

        //! Array capacity
        constexpr inline static size_t capacity() noexcept
        {
            return Capacity;
        }

        //! Load data
        inline void load(
            const char* data,
            size_t size
        )
        {
            m_externalDataPtr=nullptr;
            detail::FixedByteArrayTraits<type,ThrowOnOverflow>::checkSize(size);
            if (size>0)
            {
                std::copy(data,data+size,privData());
            }
            m_size=size;
        }

        //! Load data
        inline void load(
            char* data,
            size_t size
        )
        {
            load(const_cast<const char*>(data),size);
        }

        //! Load from null-terminated c-string
        inline void load(const char* data)
        {
            m_externalDataPtr=nullptr;
            clear();
            append(data);
        }

        //! Load from null-terminated c-string
        inline void load(char* data)
        {
            load(const_cast<const char*>(data));
        }

        //! Load from string
        inline void load(
            const std::string& str
        )
        {
            load(str.data(),str.size());
        }

        //! Load from object with data() and size() methods
        template <typename T> inline void load(const T& other)
        {
            load(other.data(),other.size());
        }

        //! Load data inline
        template <typename DataT>
        void loadInline(
            DataT* data, //!< Data buffer
            size_t size //!< Data size
        )
        {
            detail::FixedByteArrayTraits<type,ThrowOnOverflow>::checkSize(size);
            m_externalDataPtr=const_cast<char*>(data);
            m_size=size;
        }

        //! Reset array
        inline void reset() noexcept
        {
            clear();
            m_externalDataPtr=nullptr;
        }

        /** Clear array
         *  Clears content but doesn't deallocate buffer
         */
        inline void clear() noexcept
        {
            m_size=0;
        }

        //! Append data to array
        inline FixedByteArray& append(
            const char* data,
            size_t size
        )
        {
            if (size==0)
            {
                return *this;
            }
            auto newSize=m_size+size;
            detail::FixedByteArrayTraits<type,ThrowOnOverflow>::checkSize(newSize);
            auto actualSize=newSize-m_size;
            std::copy(data,data+actualSize,privData()+m_size);
            m_size=newSize;
            return *this;
        }

        //! Append null-terminated c-string to string
        inline FixedByteArray& append(
            const char* data
        )
        {
            bool overflow=false;
            while (data!=nullptr && *data!='\0')
            {
                append(*data++,&overflow);
                if (overflow)
                {
                    break;
                }
            }
            return *this;
        }

        //! Append null-terminated c-string to string
        inline FixedByteArray& append(
            char* data
        )
        {
            return append(const_cast<const char*>(data));
        }

        //! Append a char to string
        inline FixedByteArray& append(
            char ch,
            bool* overflow=nullptr
        )
        {
            auto newSize=m_size+1;
            if (!detail::FixedByteArrayTraits<type,ThrowOnOverflow>::checkSize(newSize))
            {
                if (overflow)
                {
                    *overflow=true;
                }
                return *this;
            }
            *(data()+m_size)=ch;
            m_size=newSize;
            return *this;
        }

        //! Append other
        template <typename T> inline
        FixedByteArray& append(const T& other)
        {
            return append(other.data(),other.size());
        }

        //! Append other
        FixedByteArray& append(const FixedByteArray& other)
        {
            if (other.size()==0)
            {
                return *this;
            }
            if (this==&other)
            {
                size_t prevSize=size();
                resize(prevSize*2);
                std::copy(data(),data()+prevSize,data()+prevSize);
                return *this;
            }
            return append(other.data(),other.size());
        }

        //! Push back a char
        inline FixedByteArray& push_back(
            char data
        )
        {
            return append(data);
        }

        //! Overload == operator with char*
        inline bool operator == (const char* data) const
        {
            bool ok=m_size==strlen(data)&&(memcmp(privDataConst(),data,m_size)==0);
            return ok;
        }

        //! Overload != operator with char*
        inline bool operator != (const char* data) const
        {
            return !(*this==data);
        }

        //! @todo Test comparison operators.

        //! Overload == operator
        template <typename T>
        inline bool operator == (const T& other) const noexcept
        {
            return isEqual(other.data(),other.size());
        }

        //! Overload != operator
        template <typename T>
        inline bool operator != (const T& other) const noexcept
        {
            return !isEqual(other.data(),other.size());
        }

        //! Overload < operator
        template <typename T>
        inline bool operator <(const T& other) const noexcept
        {
            if (m_size==other.size())
            {
                return memcmp(data(),other.data(),m_size)<0;
            }
            return m_size<other.size();
        }

        //! Overload > operator
        template <typename T>
        inline bool operator >(const T& other) const noexcept
        {
            if (m_size==other.size())
            {
                return memcmp(data(),other.data(),m_size)>0;
            }
            return m_size>other.size();
        }

        //! Overload >= operator
        template <typename T>
        inline bool operator >=(const T& other) const noexcept
        {
            if (m_size>other.size())
            {
                return true;
            }
            else if (m_size<other.size())
            {
                return false;
            }

            return memcmp(data(),other.data(),m_size)>=0;
        }

        //! Overload >= operator
        template <typename T>
        inline bool operator <=(const T& other) const noexcept
        {
            if (m_size<other.size())
            {
                return true;
            }
            else if (m_size>other.size())
            {
                return false;
            }

            return memcmp(data(),other.data(),m_size)<=0;
        }

        //! Get string view
        inline lib::string_view stringView() const noexcept
        {
            return lib::string_view(privDataConst(),m_size);
        }

        //! Get string view
        inline operator lib::string_view() const noexcept
        {
            return stringView();
        }

        //! Get string view of array's part
        inline lib::string_view stringView(size_t offset, size_t length=0) const noexcept
        {
            if (size()<(offset+length))
            {
                return lib::string_view();
            }

            length=(length==0)?(size()-offset):length;
            return lib::string_view(privDataConst()+offset,length);
        }

        //! Check if content is equal to some data
        inline bool isEqual(const char* data, size_t size) const noexcept
        {
            bool ok=m_size==size&&(memcmp(privDataConst(),data,m_size)==0);
            return ok;
        }

        //! Check if content is less than some data
        inline bool isLess(const char* data, size_t size) const noexcept
        {
            if (m_size==size)
            {
                return memcmp(privDataConst(),data,size)<0;
            }
            return m_size<size;
        }

        // friend inline bool operator <(const FixedByteArray& left,const FixedByteArray& right) noexcept
        // {
        //     if (left.m_size==right.m_size)
        //     {
        //         return memcmp(left.data(),right.data(),left.m_size)<0;
        //     }
        //     return left.m_size<right.m_size;
        // }

        //! Check if array is empty
        inline bool isEmpty() const noexcept
        {
            return m_size==0;
        }

        inline bool empty() const noexcept
        {
            return isEmpty();
        }

        //! Get null-terminated C-string representation
        inline const char* c_str() const noexcept
        {
            if (!m_externalDataPtr)
            {
                *(data()+m_size)='\0';
            }
            return data();
        }

        //! Resize array
        inline void resize(size_t size)
        {
            detail::FixedByteArrayTraits<type,ThrowOnOverflow>::checkSize(size);
            m_size=size;
        }

        //! Create std::string
        inline std::string toStdString() const
        {
            return std::string(data(),size());
        }

        //! Get element at position
        inline const char& at(std::size_t index) const
        {
            if (index>=size())
            {
                throw std::out_of_range("Index exceeds size of FixedByteArray");
            }
            return *(privDataConst()+index);
        }
        //! Get element at position
        inline char& at(std::size_t index)
        {
            if (index>=size())
            {
                throw std::out_of_range("Index exceeds size of FixedByteArray");
            }
            return *(privData()+index);
        }

        //! Get first element
        inline const char& first() const
        {
            return at(0);
        }
        //! Get first element
        inline char& first()
        {
            return at(0);
        }

        //! Get last element
        inline const char& last() const
        {
            if (!isEmpty())
            {
                return at(size()-1);
            }
            return at(0);
        }
        //! Get last element
        inline char& last()
        {
            if (!isEmpty())
            {
                return at(size()-1);
            }
            return at(0);
        }

        //! Overload operator []
        inline const char& operator[] (std::size_t index) const
        {
            return at(index);
        }
        //! Overload operator []
        inline char& operator[] (std::size_t index)
        {
            return at(index);
        }

        //! Overload + operator
        template <typename T>
        inline FixedByteArray operator+ (const T& other) const
        {
            return FixedByteArray(*this).append(other);
        }
        //! Overload + operator with char*
        inline FixedByteArray operator+ (const char* data) const
        {
            return FixedByteArray(*this).append(data);
        }
        //! Overload + operator with char*
        inline FixedByteArray operator+ (char data) const
        {
            return FixedByteArray(*this).append(data);
        }

        //! Overload += operator
        template <typename T>
        inline FixedByteArray& operator+= (const T& other)
        {
            return append(other);
        }
        //! Overload + operator with char*
        inline FixedByteArray operator+= (const char* data)
        {
            return append(data);
        }
        //! Overload + operator with char*
        inline FixedByteArray operator+= (char data)
        {
            return append(data);
        }

        //! Fill container with char
        inline void fill(const char ch) noexcept
        {
            memset(data(),static_cast<int>(ch),size());
        }

        //! Fill container with char
        inline void fill(const char ch, size_t offset) noexcept
        {
            if (offset<size())
            {
                memset(data()+offset,static_cast<int>(ch),size()-offset);
            }
        }

        //! Check if it is a inline raw data buffer
        inline bool isRawBuffer() const noexcept
        {
            return m_externalDataPtr!=nullptr;
        }

        inline void reserve(size_t size)
        {
            detail::FixedByteArrayTraits<type,ThrowOnOverflow>::checkSize(size);
        }

    private:

        //! Get data buffer
        inline char* privData() const noexcept
        {
            return m_externalDataPtr?m_externalDataPtr:const_cast<decltype(m_buf)&>(m_buf).data();
        }

        //! Get data buffer
        inline char* privData() noexcept
        {
            return m_externalDataPtr?m_externalDataPtr:m_buf.data();
        }

        //! Const data buffer
        inline const char* privDataConst() const noexcept
        {
            return m_externalDataPtr?m_externalDataPtr:m_buf.data();
        }

        std::array<char,Capacity+1> m_buf;
        size_t m_size;
        char* m_externalDataPtr;
};

/**
 * @brief Managed version of ByteArray
 */
template <size_t Capacity, bool ThrowOnOverflow=false> using FixedByteArrayManaged=ManagedWrapper<FixedByteArray<Capacity,ThrowOnOverflow>>;

typedef HATN_COMMON_EXPORT FixedByteArray<8> FixedByteArray8;
typedef HATN_COMMON_EXPORT FixedByteArray<16> FixedByteArray16;
typedef HATN_COMMON_EXPORT FixedByteArray<20> FixedByteArray20;
typedef HATN_COMMON_EXPORT FixedByteArray<32> FixedByteArray32;
typedef HATN_COMMON_EXPORT FixedByteArray<40> FixedByteArray40;
typedef HATN_COMMON_EXPORT FixedByteArray<64> FixedByteArray64;
typedef HATN_COMMON_EXPORT FixedByteArray<128> FixedByteArray128;
typedef HATN_COMMON_EXPORT FixedByteArray<256> FixedByteArray256;
typedef HATN_COMMON_EXPORT FixedByteArray<512> FixedByteArray512;
typedef HATN_COMMON_EXPORT FixedByteArray<1024> FixedByteArray1024;

typedef HATN_COMMON_EXPORT FixedByteArray<8,true> FixedByteArrayThrow8;
typedef HATN_COMMON_EXPORT FixedByteArray<16,true> FixedByteArrayThrow16;
typedef HATN_COMMON_EXPORT FixedByteArray<20,true> FixedByteArrayThrow20;
typedef HATN_COMMON_EXPORT FixedByteArray<32,true> FixedByteArrayThrow32;
typedef HATN_COMMON_EXPORT FixedByteArray<40,true> FixedByteArrayThrow40;
typedef HATN_COMMON_EXPORT FixedByteArray<64,true> FixedByteArrayThrow64;
typedef HATN_COMMON_EXPORT FixedByteArray<128,true> FixedByteArrayThrow128;
typedef HATN_COMMON_EXPORT FixedByteArray<256,true> FixedByteArrayThrow256;
typedef HATN_COMMON_EXPORT FixedByteArray<512,true> FixedByteArrayThrow512;
typedef HATN_COMMON_EXPORT FixedByteArray<1024,true> FixedByteArrayThrow1024;

typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<8>> FixedByteArrayShared8;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<16>> FixedByteArrayShared16;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<20>> FixedByteArrayShared20;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<32>> FixedByteArrayShared32;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<40>> FixedByteArrayShared40;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<64>> FixedByteArrayShared64;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<128>> FixedByteArrayShared128;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<256>> FixedByteArrayShared256;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<512>> FixedByteArrayShared512;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<1024>> FixedByteArrayShared1024;

typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<8,true>> FixedByteArraySharedThrow8;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<16,true>> FixedByteArraySharedThrow16;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<20,true>> FixedByteArraySharedThrow20;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<32,true>> FixedByteArraySharedThrow32;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<40,true>> FixedByteArraySharedThrow40;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<64,true>> FixedByteArraySharedThrow64;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<128,true>> FixedByteArraySharedThrow128;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<256,true>> FixedByteArraySharedThrow256;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<512,true>> FixedByteArraySharedThrow512;
typedef HATN_COMMON_EXPORT SharedPtr<FixedByteArrayManaged<1024,true>> FixedByteArraySharedThrow1024;

namespace detail
{
    template <size_t,typename=void> struct FBASizeForString
    {};
    template <size_t Length> struct FBASizeForString<Length,std::enable_if_t<(Length<=8)>>
    {
        constexpr static const size_t value=8;
    };
    template <size_t Length> struct FBASizeForString<Length,std::enable_if_t<(Length>8) && (Length<=16)>>
    {
        constexpr static const size_t value=16;
    };
    template <size_t Length> struct FBASizeForString<Length,std::enable_if_t<(Length>16) && (Length<=20)>>
    {
        constexpr static const size_t value=20;
    };
    template <size_t Length> struct FBASizeForString<Length,std::enable_if_t<(Length>20) && (Length<=32)>>
    {
        constexpr static const size_t value=32;
    };
    template <size_t Length> struct FBASizeForString<Length,std::enable_if_t<(Length>32) && (Length<=40)>>
    {
        constexpr static const size_t value=40;
    };
    template <size_t Length> struct FBASizeForString<Length,std::enable_if_t<(Length>40) && (Length<=64)>>
    {
        constexpr static const size_t value=64;
    };
    template <size_t Length> struct FBASizeForString<Length,std::enable_if_t<(Length>64) && (Length<=128)>>
    {
        constexpr static const size_t value=128;
    };
    template <size_t Length> struct FBASizeForString<Length,std::enable_if_t<(Length>128) && (Length<=256)>>
    {
        constexpr static const size_t value=256;
    };
    template <size_t Length> struct FBASizeForString<Length,std::enable_if_t<(Length>256) && (Length<=512)>>
    {
        constexpr static const size_t value=512;
    };
    template <size_t Length> struct FBASizeForString<Length,std::enable_if_t<(Length>512) && (Length<=1024)>>
    {
        static_assert(Length<=512,"MakeFixedString() can be used only for string with up to 512 symbols");
    };
}

#define MakeFixedString(str) FixedByteArray<detail::FBASizeForString<CStrLength(str)>::value>(str)
#define MakeFixedStringThrow(str) FixedByteArray<detail::FBASizeForString<CStrLength(str)>::value,true>(str)

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

namespace std {

template <size_t Length>
struct less<HATN_COMMON_NAMESPACE::FixedByteArray<Length>>
{
    // bool operator()(const HATN_COMMON_NAMESPACE::FixedByteArray<Length>& l, const HATN_COMMON_NAMESPACE::FixedByteArray<Length>& r) const
    // {
    //     return l<r;
    // }

    // bool operator()(const HATN_COMMON_NAMESPACE::lib::string_view& l, const HATN_COMMON_NAMESPACE::FixedByteArray<Length>& r) const
    // {
    //     return !(r>=l);
    // }

    // bool operator()(const HATN_COMMON_NAMESPACE::FixedByteArray<Length>& l, const HATN_COMMON_NAMESPACE::lib::string_view& r) const
    // {
    //     return l<r;
    // }

    template <typename T1, typename T2>
    bool operator()(const T1& l, const T2& r) const
    {
        return l<r;
    }

    template <typename T1>
    bool operator()(const T1& l, const char* r) const
    {
        return l<HATN_COMMON_NAMESPACE::lib::string_view(r);
    }

    template <typename T2>
    bool operator()(const char* l, const T2& r) const
    {
        return HATN_COMMON_NAMESPACE::lib::string_view(l)<r;
    }
};

} // namespace std

#endif // HATNFIXEDBYTEARRAY_H
