/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/mq.h
  *
  *  Hatn MQ Library implements a Message Queue.
  *
  */

/****************************************************************************/

#ifndef HATNMQDEF_H
#define HATNMQDEF_H

#include <hatn/mq/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_MQ_EXPORT
#   ifdef BUILD_HATN_MQ
#       define HATN_MQ_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_MQ_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_MQ_NAMESPACE_BEGIN namespace hatn { namespace mq {
#define HATN_MQ_NAMESPACE_END }}

#define HATN_MQ_NAMESPACE hatn::mq
#define HATN_MQ_NS mq
#define HATN_MQ_USING using namespace hatn::mq;

#endif // HATNMQDEF_H
