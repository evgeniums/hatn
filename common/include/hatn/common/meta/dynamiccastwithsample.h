/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/dynamiccastwithsample.h
  *
  *   Defines dynamicCastWithSample.
  *
  */

/****************************************************************************/

#ifndef HATNDYNAMICCASTWITHSAMPLE_H
#define HATNDYNAMICCASTWITHSAMPLE_H

#include <memory>
#include <type_traits>
#include <tuple>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename DerivedT, typename BaseT>
const DerivedT* dynamicCastWithSample(const BaseT* target,const DerivedT* sample) noexcept
{
    auto offset=reinterpret_cast<uintptr_t>(static_cast<DerivedT*>(const_cast<DerivedT*>(sample)))-reinterpret_cast<uintptr_t>(sample);
    auto casted=reinterpret_cast<const DerivedT*>(reinterpret_cast<uintptr_t>(target)-offset);
    return casted;
}

template <typename DerivedT, typename BaseT>
DerivedT* dynamicCastWithSample(BaseT* target,const DerivedT* sample) noexcept
{
    auto offset=reinterpret_cast<uintptr_t>(static_cast<DerivedT*>(const_cast<DerivedT*>(sample)))-reinterpret_cast<uintptr_t>(sample);
    auto casted=reinterpret_cast<DerivedT*>(reinterpret_cast<uintptr_t>(target)-offset);
    return casted;
}

HATN_COMMON_NAMESPACE_END

#endif // HATNDYNAMICCASTWITHSAMPLE_H
