/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/allocatorfactory.h
  *
  *  Factory of allocators for allocation of units and data fields
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITALLOCATORACTORY_H
#define HATNDATAUNITALLOCATORACTORY_H

#include <hatn/common/pmr/withstaticallocator.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

using AllocatorFactory=common::pmr::AllocatorFactory;

HATN_WITH_STATIC_ALLOCATOR_DECL

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNCOMMONFALLOCATORACTORY_H
