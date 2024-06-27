/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/replicatetotuple.h
  *
  * Defines replicateToTuple.
  *
  */

/****************************************************************************/

#ifndef HATNRREPLICATETOTUPLE_H
#define HATNRREPLICATETOTUPLE_H

#include <boost/hana.hpp>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename T, typename SizeT>
auto replicateToTuple(T val, SizeT size)
{
    return boost::hana::replicate<boost::hana::tuple_tag>(val,size);
}

HATN_COMMON_NAMESPACE_END

#endif // HATNRREPLICATETOTUPLE_H
