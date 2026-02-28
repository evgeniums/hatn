/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file grpcclient/grpcclient.h
  *
  */

/****************************************************************************/

#ifndef HATNGRPCCLIENT_H
#define HATNGRPCCLIENT_H

#include <hatn/app/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_GRPCCLIENT_EXPORT
#   ifdef BUILD_HATN_GRPCCLIENT
#       define HATN_GRPCCLIENT_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_GRPCCLIENT_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_GRPCCLIENT_NAMESPACE_BEGIN namespace hatn { namespace grpcclient {
#define HATN_GRPCCLIENT_NAMESPACE_END }}

HATN_GRPCCLIENT_NAMESPACE_BEGIN
HATN_GRPCCLIENT_NAMESPACE_END

#define HATN_GRPCCLIENT_NAMESPACE hatn::grpcclient
#define HATN_GRPCCLIENT_NS grpcclient
#define HATN_GRPCCLIENT_USING using namespace hatn::grpcclient;

#endif // HATNGRPCCLIENT_H
