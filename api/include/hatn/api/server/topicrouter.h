/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/topicrouter.h
  *
  */

/****************************************************************************/

#ifndef HATNAPITOPICROUTER_H
#define HATNAPITOPICROUTER_H

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

//! @todo In default topic router map topics to threads with task queues

template <typename FallbackRouterT>
class TopicRouterTraits
{
    //! @todo Route by topic
    //! Search in lru cache
    //! If not found search in external storage
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPITOPICROUTER_H
