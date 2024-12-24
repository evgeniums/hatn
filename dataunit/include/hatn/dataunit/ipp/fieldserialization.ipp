/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/ipps/fieldserialization.ipp
  *
  *      Contains definition of helper classes for serializing and deserializing dataunit fields.
  *
  */

#ifndef HATNFIELDSERIALIZATON_IPP
#define HATNFIELDSERIALIZATON_IPP

#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/fieldserialization.h>
#include <hatn/dataunit/visitors.h>

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
        rawError(RawErrorCode::END_OF_STREAM,"unexpected end of buffer at size {}",buf->size());
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
    static void load(BufferT& wired,T1 arr,const char* ptr,int dataSize, const AllocatorFactory *)
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
    static void load(BufferT& wired,common::ByteArrayShared& shared,const char* ptr,int dataSize, const AllocatorFactory *factory)
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
        const AllocatorFactory *factory,
        int maxSize,
        bool canChainBlocks
    )
{
    valBuf->clear();

    // figure out data size
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
        rawError(RawErrorCode::SUSPECT_OVERFLOW,"overflow in requested size");
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
        rawError(RawErrorCode::END_OF_STREAM,"available data size is less than requested size");
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
        auto* sizeBufSingle=wired.mainContainer();
        size_t sizeBufChainedIdx=0;
        if (wired.isSingleBuffer())
        {
            // reserve sizeof(int32_t)+1 to keep size of packed unit
            sizeBufSingle->resize(sizeBufSingle->size()+reserveSizeLength);
        }
        else
        {
            // just append meta block to keep size of packed unit
            wired.appendMetaVar(sizeof(uint32_t));
            sizeBufChainedIdx=wired.chain().size()-1;
        }
        wired.incSize(static_cast<int>(reserveSizeLength));

        // serialize field
        int size=-1;

        const auto& preparedWireData=value->wireDataKeeper();
        if (!preparedWireData.isNull())
        {
            size=wired.append(*preparedWireData);
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
                if (!sharedBuf->isEmpty())
                {
                    wired.appendBuffer(common::SpanBuffer{std::move(sharedBuf)});
                }
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
        char* sizePtr=nullptr;
        if (wired.isSingleBuffer())
        {
            sizePtr=sizeBufSingle->data();
            sizePtr+=prevSize;
        }
        else
        {
            const auto& chain=wired.chain();
            auto& item=chain.at(sizeBufChainedIdx);
            auto& sizeBufChained=item.buf;
            sizePtr=sizeBufChained.data();
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
    // figure out data size
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

    // if zero size then check required fields and finish
    if (dataSize==0)
    {
        auto failedField=io::checkRequiredFields(*value);
        if (failedField.second!=nullptr)
        {
            rawError(RawErrorCode::REQUIRED_FIELD_MISSING,failedField.first,"required field {} is not set",failedField.second);
            return false;
        }
        return true;
    }

    // check if buffer contains required amount of data
    if (buf->size()<(wired.currentOffset()+dataSize))
    {
        rawError(RawErrorCode::END_OF_STREAM,"available data size is less than requested size");
        return false;
    }

    // temporarily set WireData size to subunit size with current offset
    auto keepSize=wired.size();
    wired.setSize(wired.currentOffset()+dataSize);

    // parse subunit
    auto ok=io::deserialize(*value,wired,false);

    // restore size
    wired.setSize(keepSize);

    // done
    return ok;
}

/********************** AsBytesSer **************************/

//---------------------------------------------------------------

template <typename BufferT>
bool AsBytesSer::serializeAppend(
        BufferT& wired,
        const char* data,
        size_t size
    )
{
    wired.incSize(
        Stream<uint32_t>::packVarInt(
                wired.mainContainer(),static_cast<int>(size)
            )
        );
    wired.mainContainer()->append(data,size);
    wired.incSize(static_cast<int>(size));
    return true;
}

//---------------------------------------------------------------

template <typename BufferT>
common::DataBuf AsBytesSer::serializePrepare(
        BufferT& wired,
        size_t size
    )
{
    wired.incSize(
        Stream<uint32_t>::packVarInt(
                wired.mainContainer(),static_cast<int>(size)
            )
        );
    auto* buf=wired.mainContainer();
    auto prevSize=buf->size();
    buf->resize(prevSize+size);
    wired.incSize(static_cast<int>(size));
    return common::DataBuf{*buf,prevSize,0};
}

//---------------------------------------------------------------

template <typename BufferT, typename HandlerT>
bool AsBytesSer::deserialize(BufferT& wired, HandlerT fn, size_t maxSize)
{
    // figure out data size
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
        rawError(RawErrorCode::SUSPECT_OVERFLOW,"overflow in requested size");
        return false;
    }

    // if zero size then finish
    if (dataSize==0)
    {
        return fn(nullptr,0);
    }

    // handle data
    auto* ptr=buf->data()+wired.currentOffset();
    wired.incCurrentOffset(dataSize);
    return fn(ptr,dataSize);
}

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END

#endif // HATNFIELDSERIALIZATON_IPP
