/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file section/mac.h
  *
  */

/****************************************************************************/

#ifndef HATNMAC_H
#define HATNMAC_H

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_MAC_EXPORT
#   ifdef BUILD_HATN_MAC
#       define HATN_MAC_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_MAC_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_MAC_NAMESPACE_BEGIN namespace hatn { namespace mac {
#define HATN_MAC_NAMESPACE_END }}

#define HATN_MAC_NAMESPACE hatn::mac
#define HATN_MAC_NS mac
#define HATN_MAC_USING using namespace hatn::mac;

#endif // HATNMAC_H
