/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/detaul/unitmeta.ipp
  *
  *  Contains definitions of some meta function and types for data unit generation.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITMETA_IPP
#define HATNDATAUNITMETA_IPP

#include <hatn/dataunit/unitmeta.h>
#include <hatn/common/pmr/withstaticallocator.ipp>

HATN_DATAUNIT_NAMESPACE_BEGIN

namespace meta {

//---------------------------------------------------------------

template <typename BaseT>
unit_t<BaseT>::unit_t(AllocatorFactory* factory):BaseT(factory)
{}

template <typename BaseT>
const Field* unit_t<BaseT>::fieldById(int id) const
{
    return this->doFieldById(id);
}

template <typename BaseT>
Field* unit_t<BaseT>::fieldById(int id)
{
    return this->doFieldById(id);
}

template <typename BaseT>
bool unit_t<BaseT>::iterateFields(const Unit::FieldVisitor& visitor)
{
    return this->iterate(visitor);
}

template <typename BaseT>
bool unit_t<BaseT>::iterateFieldsConst(const Unit::FieldVisitorConst& visitor) const
{
    return this->iterateConst(visitor);
}

template <typename BaseT>
size_t unit_t<BaseT>::fieldCount() const noexcept
{
    return this->doFieldCount();
}

template <typename BaseT>
const char* unit_t<BaseT>::name() const noexcept
{
    return this->unitName();
}

template <typename BaseT>
int unit_t<BaseT>::serialize(WireData& wired,bool topLevel) const
{
    return io::serialize(*this,wired,topLevel);
}

#if 0
//! @todo Maybe implement virtual serializrion
template <typename BaseT>
int unit_t<BaseT>::serialize(WireBufSolid& wired,bool topLevel) const
{
    return io::serialize(*this,wired,topLevel);
}

template <typename BaseT>
int unit_t<BaseT>::serialize(WireBufSolidShared& wired,bool topLevel) const
{
    return io::serialize(*this,wired,topLevel);
}

template <typename BaseT>
int unit_t<BaseT>::serialize(WireBufChained& wired,bool topLevel) const
{
    return io::serialize(*this,wired,topLevel);
}
#endif

template <typename BaseT>
bool unit_t<BaseT>::parse(WireData& wired,bool topLevel)
{
    return io::deserialize(*this,wired,topLevel);
}

template <typename BaseT>
bool unit_t<BaseT>::parse(WireBufSolid& wired,bool topLevel)
{
    return io::deserialize(*this,wired,topLevel);
}

//---------------------------------------------------------------
} // namespace meta

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITMETA_IPP
