/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/detail/wirebuf.ipp
  *
  *  Contains definitions for wire buf templates.
  *
  */

#ifndef HATNDATAUNITWIREBUF_IPP
#define HATNDATAUNITWIREBUF_IPP

#include <boost/hana.hpp>

#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/wirebuf.h>
#include <hatn/dataunit/wirebufsolid.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/********************** WireBuf **************************/

//---------------------------------------------------------------

template <typename TraitsT>
int WireBuf<TraitsT>::appendUint32(uint32_t val)
{
    auto* buf=hana::if_(
        boost::hana::is_a<WireBufSolidTag>,
        mainContainer(),
        appendMetaVar(sizeof(uint32_t))
    );

    auto consumed=Stream<uint32_t>::packVarInt(buf,val);
    incSize(consumed);
    return consumed;
}

//---------------------------------------------------------------
#if 0
template <typename TraitsT>
template <typename T>
int WireBuf<TraitsT>::append(T* other)
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
            auto&& sharedBuf=factory()->template createObject<::hatn::common::ByteArrayManaged>(
                                srcContainer->data(),srcContainer->size(),factory()->dataMemoryResource()
                            );
            appendBuffer(std::move(sharedBuf));
        }

        if (!other->isSingleBuffer())
        {
            // copy chain of buffers
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
#endif

//---------------------------------------------------------------

template <typename TraitsT>
WireBufSolid WireBuf<TraitsT>::toSolidWireBuf() const
{
    auto f=factory();
    common::pmr::memory_resource* memResource=f?f->dataMemoryResource():common::pmr::get_default_resource();
    common::ByteArray singleBuf(memResource);
    copyToContainer(singleBuf);
    return WireBufSolid(std::move(singleBuf),f);
}

template <typename TraitsT>
WireBufSolid WireBuf<TraitsT>::toSingleWireData() const
{
    return toSolidWireBuf();
}

//---------------------------------------------------------------

template <typename T>
WireBufSolid::WireBufSolid(
        WireDataDerived<T>&& buf
    ) : WireBufSolid(buf.toSolidWireBuf())
{}

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREBUF_IPP
