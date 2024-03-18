/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtree.h
  *
  * Contains declarations of configuration tree types and functions.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGTREE_H
#define HATNCONFIGTREE_H


#include <hatn/base/base.h>
#include <hatn/base/baseerror.h>
#include <hatn/base/configtreevalue.h>
#include <hatn/base/detail/configtreevalue.ipp>

HATN_BASE_NAMESPACE_BEGIN

class HATN_BASE_EXPORT ConfigTree : public ConfigTreeValue
{
    public:

        using ConfigTreeValue::ConfigTreeValue;
};


HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREE_H
