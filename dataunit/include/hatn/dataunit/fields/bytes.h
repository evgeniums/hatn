/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/bytes.h
  *
  *      DataUnit fields for bytes and strings
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITBYTEFIELDS_H
#define HATNDATAUNITBYTEFIELDS_H

#include <boost/algorithm/string/predicate.hpp>

#include <hatn/dataunit/fields/fieldtraits.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

template <typename T,typename T2>
struct _BytesFieldHelper
{
    static void store(WireData& wired, T arr, T2)
    {
        wired.mainContainer()->append(arr->data(),arr->size());
        wired.incSize(static_cast<int>(arr->size()));
    }

    static void load(WireData&,T arr,const char* ptr,int dataSize, const AllocatorFactory *)
    {
        arr->load(ptr,dataSize);
    }
};

template <>
struct _BytesFieldHelper<::hatn::common::ByteArrayShared,::hatn::common::ByteArray>
{
    static void store(WireData& wired,const ::hatn::common::ByteArrayShared& shared,const ::hatn::common::ByteArray& onstack)
    {
        if (!shared.isNull())
        {
            wired.appendBuffer(shared);
            wired.incSize(static_cast<int>(shared->size()));
        }
        else
        {
            auto&& sharedBlock=wired.factory()->createObject<::hatn::common::ByteArrayManaged>(onstack.data(),onstack.size(),wired.factory()->dataMemoryResource());
            wired.appendBuffer(sharedBlock);
            wired.incSize(static_cast<int>(onstack.size()));
        }
    }

    static void load(WireData&,::hatn::common::ByteArrayShared& shared,const char* ptr,int dataSize, const AllocatorFactory *factory)
    {
        shared=factory->createObject<::hatn::common::ByteArrayManaged>(ptr,dataSize,factory->dataMemoryResource());
    }
};

struct BytesType{};

//! Base field template for bulk data types
template <typename Type>
class FieldTmplBytes : public Field, public BytesType
{
    public:

        using isUnitType=std::false_type;
        using selfType=FieldTmplBytes<Type>;
        using isRepeatedType=std::false_type;
        constexpr static const bool CanChainBlocks=true;
        constexpr static auto typeId=Type::typeId;
        using type=typename Type::type;

        explicit FieldTmplBytes(
            Unit *parentUnit
            ) : Field(Type::typeId,parentUnit),
            m_factory(parentUnit->factory()),
            m_value(parentUnit)
        {
        }

        constexpr inline static WireType fieldWireType() noexcept
        {
            return WireType::WithLength;
        }
        //! Get wire type of the field
        virtual WireType wireType() const noexcept override
        {
            return fieldWireType();
        }

        //! Deserialize field from wire
        template <typename BufferT>
        static bool deserialize(typename Type::type& value, BufferT& wired, const AllocatorFactory *factory)
        {
            return BytesSer<
                        typename Type::type::onstackType,
                        typename Type::type::sharedType
                    >
                    ::
                    deserialize(
                        wired,
                        value.buf(),
                        value.byteArrayShared(),
                        factory,
                        Type::type::maxSize::value,
                        Type::type::canChainBlocks::value
                        );
        }

        //! Deserialize field from wire
        template <typename BufferT>
        bool deserialize(BufferT& wired, const AllocatorFactory *factory)
        {
            this->markSet(deserialize(this->m_value,wired,factory));
            return this->isSet();
        }

        //! Serialize field to wire
        template <typename BufferT>
        static bool serialize(const typename Type::type& value, BufferT& wired)
        {
            return BytesSer<
                        typename Type::type::onstackType,
                        typename Type::type::sharedType
                    >
                    ::
                    serialize(
                        wired,
                        value.buf(),
                        value.byteArrayShared(),
                        Type::type::canChainBlocks::value
                        );
        }

        //! Serialize field to wire
        template <typename BufferT>
        bool serialize(BufferT& wired) const
        {
            return BytesSer<
                typename Type::type::onstackType,
                typename Type::type::sharedType
                >
                ::
                serialize(
                    wired,
                    this->m_value.buf(),
                    this->m_value.byteArrayShared(),
                    Type::type::canChainBlocks::value
                    );
        }

        //! Get field size
        virtual size_t maxPackedSize() const noexcept override
        {
            return valueSize(m_value);
        }

        //! Get size of value
        static inline size_t valueSize(const typename Type::type& value) noexcept
        {
            return value.maxPackedSize();
        }

        size_t fieldSize() const noexcept
        {
            return valueSize(m_value);
        }

        //! Prepare shared form of value storage for parsing from wire
        inline static void prepareSharedStorage(typename Type::type& value,const AllocatorFactory* factory)
        {
            if (value.byteArrayShared().isNull())
            {
                value.setSharedByteArray(Type::type::createShared(factory));
            }
        }

        //! Set data from buffer
        inline void set(const char* ptr,size_t size)
        {
            buf()->load(ptr,size);
        }

        //! Set data from null-terminated c-string
        inline void set(const char* ptr)
        {
            buf()->load(ptr);
        }

        //! Set data from string
        inline void set(
            const std::string& str
        )
        {
            buf()->load(str);
        }

        //! Set data from container
        template <typename ContainerT>
        inline void set(
            const ContainerT& container
        )
        {
            buf()->load(container);
        }

        inline void set(typename Type::type::sharedType val)
        {
            markSet();
            this->m_value.setSharedByteArray(std::move(val));
        }

        //! Get C-string
        inline const char* c_str() const noexcept
        {
            return buf()->c_str();
        }

        //! Get pointer to data
        inline char* dataPtr() noexcept
        {
            return buf()->data();
        }

        //! Get pointer to data
        inline char* dataPtr() const noexcept
        {
            return buf()->data();
        }

        //! Get data size
        inline size_t dataSize() const noexcept
        {
            return buf()->size();
        }

        inline auto value() const noexcept
        {
            return lib::string_view{dataPtr(),dataSize()};
        }

        //! Clear field
        virtual void clear() override
        {
            fieldClear();
        }

        //! Clear field
        void fieldClear()
        {
            this->m_value.clear();
        }

        //! Reset field
        virtual void reset() override
        {
            fieldReset();
        }

        //! Reset field
        void fieldReset()
        {
            this->m_value.reset();
            this->markSet(false);
        }

        inline typename Type::type::onstackType& byteArray() noexcept
        {
            return this->m_value.byteArray();
        }

        inline typename Type::type::sharedType& byteArrayShared() noexcept
        {
            return this->m_value.byteArrayShared();
        }

        inline const typename Type::type::onstackType& byteArray() const noexcept
        {
            return this->m_value.byteArray();
        }

        inline const typename Type::type::sharedType& byteArrayShared() const noexcept
        {
            return this->m_value.byteArrayShared();
        }

        inline void setSharedByteArray(typename Type::type::sharedType val)
        {
            this->m_value.setSharedByteArray(std::move(val));
        }

        inline typename Type::type::onstackType* buf(bool forSet=true) noexcept
        {
            if (forSet)
            {
                markSet();
            }
            return this->m_value.buf();
        }

        inline typename Type::type::onstackType* mutableValue() noexcept
        {
            return buf(true);
        }

        inline const typename Type::type::onstackType* buf() const noexcept
        {
            return this->m_value.buf();
        }

        //! Load data from buffer
        inline void load(const char* ptr,size_t size)
        {
            buf()->load(ptr,size);
        }

        //! Append data to buffer
        inline void append(const char* ptr,size_t size)
        {
            buf()->append(ptr,size);
        }

        //! Append data to buffer
        template <typename ContainerT>
        inline void append(const ContainerT& container)
        {
            buf()->append(container);
        }

        //! Get pointer to data
        inline char* data() noexcept
        {
            return buf()->data();
        }

        //! Get pointer to data
        inline char* data() const noexcept
        {
            return buf()->data();
        }

        //! Get data size
        inline size_t size() const noexcept
        {
            return buf()->size();
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

        //! Get char by index
        inline const char& at(std::size_t index) const
        {
            return buf()->at(index);
        }

        //! Get char by index
        inline char& at(std::size_t index)
        {
            return buf()->at(index);
        }

        /**
         * @brief Use shared version of byte arrays data when parsing wired data
         * @param enable Enabled on/off
         *
         * When enabled then shared byte arrays will be auto allocated in managed shared buffers
         */
        virtual void setParseToSharedArrays(bool enable,const AllocatorFactory* factory=nullptr) override
        {
            fieldSetParseToSharedArrays(enable,factory);
        }

        /**
         * @brief Check if shared byte arrays must be used for parsing
         * @return Boolean flag
         */
        virtual bool isParseToSharedArrays() const noexcept override
        {
            return fieldIsParseToSharedArrays();
        }

        void fieldSetParseToSharedArrays(bool enable,const AllocatorFactory* factory)
        {
            if (enable)
            {
                if (factory==nullptr)
                {
                    factory=m_factory;
                    if (factory==nullptr)
                    {
                        factory=AllocatorFactory::getDefault();
                    }
                }
                prepareSharedStorage(this->m_value,factory);
            }
            else
            {
                byteArrayShared().reset();
            }
        }

        bool fieldIsParseToSharedArrays() const noexcept
        {
            return !byteArrayShared().isNull();
        }


        //! Format as JSON element
        inline static bool formatJSON(const typename Type::type& value,json::Writer* writer)
        {
            if (Type::isStringType::value)
            {
                return writer->String(value.buf()->data(),value.buf()->size());
            }
            return writer->RawBase64(value.buf()->data(),value.buf()->size());
        }

        //! Serialize as JSON element
        virtual bool toJSON(json::Writer* writer) const override
        {
            return formatJSON(this->m_value,writer);
        }

        virtual void pushJsonParseHandler(Unit *topUnit) override
        {
            json::pushHandler<selfType,json::FieldReader<Type,selfType>>(topUnit,this);
        }

        //! Can chain blocks
        virtual bool canChainBlocks() const noexcept override
        {
            return CanChainBlocks;
        }

        //! Can chain blocks
        constexpr static bool fieldCanChainBlocks() noexcept
        {
            return CanChainBlocks;
        }

        using Field::less;
        using Field::equals;

        virtual bool less(const char* data, size_t length) const override{return buf()->isLess(data,length);}
        virtual bool equals(const char* data, size_t length) const override {return buf()->isEqual(data,length);}

        virtual bool lexLess(const char* data, size_t length) const override
        {
            return boost::algorithm::lexicographical_compare(buf()->stringView(),common::lib::string_view(data,length));
        }
        virtual bool lexLessI(const char* data, size_t length) const override
        {
            return boost::algorithm::ilexicographical_compare(buf()->stringView(),common::lib::string_view(data,length));
        }
        virtual bool lexEquals(const char* data, size_t length) const override
        {
            return boost::algorithm::equals(buf()->stringView(),common::lib::string_view(data,length));
        }
        virtual bool lexEqualsI(const char* data, size_t length) const override
        {
            return boost::algorithm::iequals(buf()->stringView(),common::lib::string_view(data,length));
        }

        virtual void setV(const char* data, size_t length) override {this->markSet();buf()->load(data,length);}
        virtual void bufResize(size_t size) override {this->markSet();buf()->resize(size);}
        virtual void bufReserve(size_t size) override {buf()->reserve(size);}
        virtual const char* bufCStr() override {return c_str();}
        virtual char* bufData() const override {return dataPtr();}
        virtual char* bufData() override {return dataPtr();}
        virtual size_t bufSize() const override {return dataSize();}
        virtual void bufCreateShared() override {this->markSet();prepareSharedStorage(m_value,m_factory);}
        virtual size_t bufCapacity() const override {return buf()->capacity();}
        virtual bool bufEmpty() const override {return buf()->isEmpty();}

    protected:

        //! Load field from wire
        virtual bool doLoad(WireData& wired, const AllocatorFactory* factory) override
        {
            return deserialize(this->m_value,wired,factory);
        }

        //! Store field to wire
        virtual bool doStore(WireData& wired) const override
        {
            return serialize(this->m_value,wired);
        }

        const AllocatorFactory* m_factory;
        typename Type::type m_value;
};

template<>
struct FieldTmpl<TYPE_BYTES> : public FieldTmplBytes<TYPE_BYTES>
{
    using FieldTmplBytes<TYPE_BYTES>::FieldTmplBytes;
};

template<>
struct FieldTmpl<TYPE_STRING> : public FieldTmplBytes<TYPE_STRING>
{
    using FieldTmplBytes<TYPE_STRING>::FieldTmplBytes;
};

template <template <int> class _S,int Length>
struct FieldTmpl<_S<Length>> : public FieldTmplBytes<_S<Length>>
{
    using FieldTmplBytes<_S<Length>>::FieldTmplBytes;
    constexpr static const bool CanChainBlocks=false;
    virtual bool canChainBlocks() const noexcept override
    {
        return CanChainBlocks;
    }
    constexpr static bool fieldCanChainBlocks() noexcept
    {
        return CanChainBlocks;
    }
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITBYTEFIELDS_H
