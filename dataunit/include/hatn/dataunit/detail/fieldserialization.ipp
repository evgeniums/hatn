/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/details/fieldserialization.ipp
  *
  *      Contains definition of helper classes for serializing and deserializing dataunit fields.
  *
  */

#ifndef HATNFIELDSERIALIZATON_IPP
#define HATNFIELDSERIALIZATON_IPP

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/fieldserialization.h>
#include <hatn/dataunit/visitors/serialize.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/********************** FixedSer **************************/

//---------------------------------------------------------------
template <typename T>
struct Endianity
{
    static void littleToNative(T& val)
    {
        boost::endian::little_to_native_inplace(val);
    }

    static void nativeToLittle(T& val)
    {
        boost::endian::native_to_little_inplace(val);
    }
};
template <>
struct Endianity<float>
{
    static void littleToNative(float& val)
    {
        uint32_t* intVal=reinterpret_cast<uint32_t*>(&val);
        boost::endian::little_to_native_inplace(*intVal);
    }

    static void nativeToLittle(float& val)
    {
        uint32_t* intVal=reinterpret_cast<uint32_t*>(&val);
        boost::endian::native_to_little_inplace(*intVal);
    }
};
template <>
struct Endianity<double>
{
    static void nativeToLittle(double& val)
    {
        uint64_t* intVal=reinterpret_cast<uint64_t*>(&val);
        boost::endian::little_to_native_inplace(*intVal);
    }

    static void littleToNative(double& val)
    {
        uint64_t* intVal=reinterpret_cast<uint64_t*>(&val);
        boost::endian::native_to_little_inplace(*intVal);
    }
};

//---------------------------------------------------------------
template <typename T>
template <typename BufferT>
bool FixedSer<T>::serialize(const T& value, BufferT& wired)
{
    constexpr static const int valueSize=sizeof(T);

    // prepare
    auto* buf=wired.mainContainer();
    auto initialSize=buf->size();
    buf->resize(initialSize+valueSize);
    auto* ptr=buf->data()+initialSize;

    // copy data
    memcpy(ptr,&value,valueSize);

    // respect endianity: on wire it is litle endian
    Endianity<T>::nativeToLittle(*reinterpret_cast<T*>(ptr));

    // ok
    wired.incSize(valueSize);
    return true;
}

//---------------------------------------------------------------
template <typename T>
template <typename BufferT>
bool FixedSer<T>::deserialize(T& value, BufferT& wired)
{
    constexpr static const int valueSize=sizeof(T);

    // prepare
    auto* buf=wired.mainContainer();
    auto* ptr=buf->data()+wired.currentOffset();

    // check overflow
    wired.incCurrentOffset(valueSize);
    if (wired.currentOffset()>buf->size())
    {
        HATN_WARN(dataunit,"Unexpected end of buffer")
        return false;
    }

    // copy data
    memcpy(&value,ptr,valueSize);

    // respect endianity: on wire it is litle endian
    Endianity<T>::littleToNative(value);

    // ok
    return true;
}

/********************** VariableSer **************************/

//---------------------------------------------------------------
template <typename T>
template <typename BufferT>
bool VariableSer<T>::serialize(const T& value, BufferT& wired)
{
    auto* buf=wired.mainContainer();
    auto consumed=Stream<T>::packVarInt(buf,value);
    if (consumed<0)
    {
        return false;
    }
    wired.incSize(consumed);
    return true;
}

//---------------------------------------------------------------
template <typename T>
template <typename BufferT>
bool VariableSer<T>::deserialize(T& value, BufferT& wired)
{
    auto* buf=wired.mainContainer();
    size_t availableSize=buf->size()-wired.currentOffset();
    auto consumed=Stream<T>::unpackVarInt(buf->data()+wired.currentOffset(),availableSize,value);
    if (consumed<0)
    {
        return false;
    }
    wired.incCurrentOffset(consumed);
    return true;
}

#if 0
template class HATN_DATAUNIT_EXPORT FixedSer<int32_t>;
template class HATN_DATAUNIT_EXPORT FixedSer<int64_t>;
template class HATN_DATAUNIT_EXPORT FixedSer<uint32_t>;
template class HATN_DATAUNIT_EXPORT FixedSer<uint64_t>;
template class HATN_DATAUNIT_EXPORT FixedSer<float>;
template class HATN_DATAUNIT_EXPORT FixedSer<double>;

template class HATN_DATAUNIT_EXPORT VariableSer<bool>;
template class HATN_DATAUNIT_EXPORT VariableSer<int8_t>;
template class HATN_DATAUNIT_EXPORT VariableSer<int16_t>;
template class HATN_DATAUNIT_EXPORT VariableSer<int32_t>;
template class HATN_DATAUNIT_EXPORT VariableSer<int64_t>;
template class HATN_DATAUNIT_EXPORT VariableSer<uint8_t>;
template class HATN_DATAUNIT_EXPORT VariableSer<uint16_t>;
template class HATN_DATAUNIT_EXPORT VariableSer<uint32_t>;
template class HATN_DATAUNIT_EXPORT VariableSer<uint64_t>;

#endif

/********************** BytesSer **************************/

//---------------------------------------------------------------
template <typename T1,typename T2>
struct _BytesSer
{
    template <typename BufferT>
    static void store(BufferT& wired, T1 arr, T2)
    {
        Assert(arr,"Empty arr");
        wired.mainContainer()->append(arr->data(),arr->size());
        wired.incSize(static_cast<int>(arr->size()));
    }

    template <typename BufferT>
    static void load(BufferT& wired,T1 arr,const char* ptr,int dataSize, AllocatorFactory *)
    {
        if (wired.isUseInlineBuffers())
        {
            arr->loadInline(ptr,dataSize);
        }
        else
        {
            arr->load(ptr,dataSize);
        }
    }
};

template <>
struct _BytesSer<const common::ByteArrayShared&,const common::ByteArray*>
{

    template <typename BufferT>
    static void store(
            BufferT& wired,
            const common::ByteArrayShared& shared,
            const common::ByteArray* onstack
        )
    {
        if (!shared.isNull())
        {
            wired.appendBuffer(shared);
            wired.incSize(static_cast<int>(shared->size()));
        }
        else
        {
            auto&& f=wired.factory();
            auto&& sharedBuf=f->template createObject<common::ByteArrayManaged>(onstack->data(),onstack->size(),f->dataMemoryResource());
            wired.appendBuffer(std::move(sharedBuf));
            wired.incSize(static_cast<int>(onstack->size()));
        }
    }
};

template <>
struct _BytesSer<common::ByteArrayShared&,common::ByteArray*>
{
    template <typename BufferT>
    static void load(BufferT& wired,common::ByteArrayShared& shared,const char* ptr,int dataSize, AllocatorFactory *factory)
    {
        shared=factory->createObject<common::ByteArrayManaged>(ptr,dataSize,wired.isUseInlineBuffers(),factory->dataMemoryResource());
    }
};

//---------------------------------------------------------------
template <typename onstackT,typename sharedT>
template <typename BufferT>
bool BytesSer<onstackT,sharedT>::serialize(
        BufferT& wired,
        const onstackT* buf,
        const sharedT& shared,
        bool canChainBlocks
    )
{
    // append data size
    auto&& dataSize=buf->size();
    if (!wired.isSingleBuffer() && canChainBlocks)
    {
        wired.appendUint32(static_cast<uint32_t>(dataSize));
    }
    else
    {
        wired.incSize(Stream<uint32_t>::packVarInt(wired.mainContainer(),static_cast<int>(dataSize)));
    }

    // zero data ok
    if (dataSize==0)
    {
        return true;
    }

    // append data
    if (!wired.isSingleBuffer() && canChainBlocks)
    {
        _BytesSer<const sharedT&,const onstackT*>::store(wired,shared,buf);
    }
    else
    {
        _BytesSer<const onstackT*,const onstackT*>::store(wired,buf,buf);
    }

    // ok
    return true;
}

//---------------------------------------------------------------
template <typename onstackT,typename sharedT>
template <typename BufferT>
bool BytesSer<onstackT,sharedT>::deserialize(
        BufferT& wired,
        onstackT* valBuf,
        sharedT& shared,
        AllocatorFactory *factory,
        int maxSize,
        bool canChainBlocks
    )
{
    valBuf->clear();

    // find data size
    uint32_t dataSize=0;
    auto* buf=wired.mainContainer();
    size_t availableBufSize=buf->size()-wired.currentOffset();
    auto consumed=Stream<uint32_t>::unpackVarInt(buf->data()+wired.currentOffset(),availableBufSize,dataSize);
    if (consumed<0)
    {
        return false;
    }
    wired.incCurrentOffset(consumed);

    // check if size is more than allowed
    if (maxSize>0&&static_cast<int>(dataSize)>static_cast<int>(maxSize))
    {
        HATN_WARN(dataunit,"Overflow in size of bytes buffer")
        return false;
    }

    // if zero size then finish
    if (dataSize==0)
    {
        return true;
    }
    auto* ptr=buf->data()+wired.currentOffset();
    wired.incCurrentOffset(dataSize);

    // check if buffer contains required amount of data
    if (buf->size()<wired.currentOffset())
    {
        HATN_WARN(dataunit,"Size of bytes buffer is less than requested size")
        return false;
    }

    // copy data
    if (wired.isUseSharedBuffers() && canChainBlocks)
    {
        _BytesSer<sharedT&,onstackT*>::load(wired,shared,ptr,dataSize,factory);
    }
    else
    {
        _BytesSer<onstackT*,onstackT*>::load(wired,valBuf,ptr,dataSize,factory);
    }

    // ok
    return true;
}

#if 0
template class HATN_DATAUNIT_EXPORT BytesSer<common::ByteArray,common::ByteArrayShared>;
template class HATN_DATAUNIT_EXPORT BytesSer<common::FixedByteArrayThrow8,common::FixedByteArraySharedThrow8>;
template class HATN_DATAUNIT_EXPORT BytesSer<common::FixedByteArrayThrow16,common::FixedByteArraySharedThrow16>;
template class HATN_DATAUNIT_EXPORT BytesSer<common::FixedByteArrayThrow20,common::FixedByteArraySharedThrow20>;
template class HATN_DATAUNIT_EXPORT BytesSer<common::FixedByteArrayThrow32,common::FixedByteArraySharedThrow32>;
template class HATN_DATAUNIT_EXPORT BytesSer<common::FixedByteArrayThrow40,common::FixedByteArraySharedThrow40>;
template class HATN_DATAUNIT_EXPORT BytesSer<common::FixedByteArrayThrow64,common::FixedByteArraySharedThrow64>;
template class HATN_DATAUNIT_EXPORT BytesSer<common::FixedByteArrayThrow128,common::FixedByteArraySharedThrow128>;
template class HATN_DATAUNIT_EXPORT BytesSer<common::FixedByteArrayThrow256,common::FixedByteArraySharedThrow256>;
template class HATN_DATAUNIT_EXPORT BytesSer<common::FixedByteArrayThrow512,common::FixedByteArraySharedThrow512>;
template class HATN_DATAUNIT_EXPORT BytesSer<common::FixedByteArrayThrow1024,common::FixedByteArraySharedThrow1024>;
#endif

/********************** UnitSer **************************/

//---------------------------------------------------------------

template <typename UnitT, typename BufferT>
bool UnitSer::serialize(const UnitT* value, BufferT& wired)
{
    if (value!=nullptr)
    {
        size_t prevSize=wired.size();

        // prepare buffer for size of the packed unit
        size_t reserveSizeLength=sizeof(uint32_t)+1;
        auto* sizeBuf=wired.mainContainer();
        if (wired.isSingleBuffer())
        {
            // reserve sizeof(int32_t)+1 to keep size of packed unit
            sizeBuf->resize(sizeBuf->size()+reserveSizeLength);
        }
        else
        {
            // just append meta block to keep size of packed unit
            sizeBuf=wired.appendMetaVar(sizeof(uint32_t));
        }
        wired.incSize(static_cast<int>(reserveSizeLength));

        // serialize field
        int size=-1;

        const auto& preparedWireDataPack=value->wireDataPack();
        if (!preparedWireDataPack.isNull())
        {
            size=wired.append(preparedWireDataPack->wireData());
        }
        else
        {
            if (!wired.isSingleBuffer())
            {
                auto&& f=wired.factory();
                auto&& sharedBuf=f->template createObject<::hatn::common::ByteArrayManaged>(
                    f->dataMemoryResource()
                    );
                wired.setCurrentMainContainer(sharedBuf.get());
                auto keepOffset=wired.currentOffset();
                wired.setCurrentOffset(0);

                size=io::serialize(*value,wired,false);
                if (size<0)
                {
                    return false;
                }

                wired.setCurrentOffset(keepOffset);
                wired.setCurrentMainContainer(nullptr);
                wired.appendBuffer(std::move(sharedBuf));
            }
            else
            {
                size=io::serialize(*value,wired,false);
                if (size<0)
                {
                    return false;
                }
            }
        }

        // preset size of embedded unit with 0
        char* sizePtr=sizeBuf->data();
        if (wired.isSingleBuffer())
        {
            sizePtr+=prevSize;
        }
        memset(sizePtr,0,reserveSizeLength);

        // pack size and set MSB for each byte so it won't be dropped
        StreamBase::packVarInt32(sizePtr,size);
        for (size_t i=0;i<(reserveSizeLength-1);i++)
        {
            sizePtr[i]|=static_cast<char>(0x80);
        }

        // ok
        return true;
    }
    return false;
}

//---------------------------------------------------------------

template <typename UnitT, typename BufferT>
bool UnitSer::deserialize(UnitT* value, BufferT& wired)
{
    // find data size
    uint32_t dataSize=0;
    auto* buf=wired.mainContainer();
    size_t availableBufSize=buf->size()-wired.currentOffset();
    auto ptr=buf->data()+wired.currentOffset();
    auto consumed=Stream<uint32_t>::unpackVarInt(ptr,availableBufSize,dataSize);
    if (consumed<0)
    {
        return false;
    }
    wired.incCurrentOffset(consumed);

    // if zero size then finish
    if (dataSize==0)
    {
        value->clear();
        return true;
    }

    // check if buffer contains required amount of data
    if (buf->size()<(wired.currentOffset()+dataSize))
    {
        HATN_WARN(dataunit,"Size of bytes buffer is less than requested size")
        return false;
    }

    // temporarily set WireData size to SubUnit size with current offset
    auto keepSize=wired.size();
    wired.setSize(wired.currentOffset()+dataSize);

    // parse SubUnit
    auto ok=io::deserialize(*value,wired,false);
    // auto ok=value->parse(wired,false);

    // restore size
    wired.setSize(keepSize);

    // done
    return ok;
}


//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END

#endif // HATNFIELDSERIALIZATON_IPP
