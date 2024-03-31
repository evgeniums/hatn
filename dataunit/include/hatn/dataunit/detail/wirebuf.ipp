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
    auto self=this;
    auto* buf=boost::hana::eval_if(
        boost::hana::bool_<WireBuf<TraitsT>::isSingleBuffer()>{},
        [&](auto _)
        {
            return _(self)->mainContainer();
        },
        [&](auto _)
        {
            return _(self)->appendMetaVar(sizeof(uint32_t));
        }
    );

    auto consumed=Stream<uint32_t>::packVarInt(buf,val);
    incSize(consumed);
    return consumed;
}

//---------------------------------------------------------------

template <typename TraitsT>
WireBufSolid WireBuf<TraitsT>::toSolidWireBuf() const
{
    auto f=factory();
    common::pmr::memory_resource* memResource=f?f->dataMemoryResource():common::pmr::get_default_resource();
    common::ByteArray singleBuf(memResource);
    copyToContainer(*this,&singleBuf);
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
