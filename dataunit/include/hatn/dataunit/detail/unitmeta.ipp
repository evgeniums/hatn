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

template <typename BaseT, typename UniqueType>
unit_t<BaseT,UniqueType>::unit_t(AllocatorFactory* factory):BaseT(factory)
{}

template <typename BaseT, typename UniqueType>
const Field* unit_t<BaseT,UniqueType>::fieldById(int id) const
{
    return this->doFieldById(id);
}

template <typename BaseT, typename UniqueType>
Field* unit_t<BaseT,UniqueType>::fieldById(int id)
{
    return this->doFieldById(id);
}

template <typename BaseT, typename UniqueType>
bool unit_t<BaseT,UniqueType>::iterateFields(const Unit::FieldVisitor& visitor)
{
    return this->iterate(visitor);
}

template <typename BaseT, typename UniqueType>
bool unit_t<BaseT,UniqueType>::iterateFieldsConst(const Unit::FieldVisitorConst& visitor) const
{
    return this->iterateConst(visitor);
}

template <typename BaseT, typename UniqueType>
size_t unit_t<BaseT,UniqueType>::fieldCount() const noexcept
{
    return this->doFieldCount();
}

template <typename BaseT, typename UniqueType>
const char* unit_t<BaseT,UniqueType>::name() const noexcept
{
    return this->unitName();
}

template <typename BaseT, typename UniqueType>
int unit_t<BaseT,UniqueType>::serialize(WireData& wired,bool topLevel) const
{
    return io::serialize(*this,wired,topLevel);
}

#if 0
//! @todo Maybe implement virtual serializrion
template <typename BaseT, typename UniqueType>
int unit_t<BaseT,UniqueType>::serialize(WireBufSolid& wired,bool topLevel) const
{
    return io::serialize(*this,wired,topLevel);
}

template <typename BaseT, typename UniqueType>
int unit_t<BaseT,UniqueType>::serialize(WireBufSolidShared& wired,bool topLevel) const
{
    return io::serialize(*this,wired,topLevel);
}

template <typename BaseT, typename UniqueType>
int unit_t<BaseT,UniqueType>::serialize(WireBufChained& wired,bool topLevel) const
{
    return io::serialize(*this,wired,topLevel);
}
#endif

template <typename BaseT, typename UniqueType>
bool unit_t<BaseT,UniqueType>::parse(WireData& wired,bool topLevel)
{
    return io::deserialize(*this,wired,topLevel);
}

template <typename BaseT, typename UniqueType>
bool unit_t<BaseT,UniqueType>::parse(WireBufSolid& wired,bool topLevel)
{
    return io::deserialize(*this,wired,topLevel);
}

//---------------------------------------------------------------
} // namespace meta

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITMETA_IPP
