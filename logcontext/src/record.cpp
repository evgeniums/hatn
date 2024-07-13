/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file logcontext/record.сpp
  *
  *  Contains definitions of record classes of log context.
  *
  */

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/record.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

namespace {

using FactoryPtr=common::pmr::AllocatorFactory**;

FactoryPtr& factory() noexcept
{
    static auto dflt=common::pmr::AllocatorFactory::getDefault();
    static FactoryPtr inst=&dflt;
    return inst;
}

}

common::pmr::AllocatorFactory* ContextAllocatorFactory::defaultFactory() noexcept
{
    return *factory();
}

void ContextAllocatorFactory::setDefaultFactory(common::pmr::AllocatorFactory* f) noexcept
{
    *factory()=f;
}

HATN_LOGCONTEXT_NAMESPACE_END
