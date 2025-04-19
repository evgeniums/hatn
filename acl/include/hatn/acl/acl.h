/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file acl/acl.h
  *
  *  Hatn ACL Library contains types and methods to use for base acllications.
  *
  */

/****************************************************************************/

#ifndef HATNACL_H
#define HATNACL_H

#include <hatn/acl/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_ACL_EXPORT
#   ifdef BUILD_HATN_ACL
#       define HATN_ACL_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_ACL_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_ACL_NAMESPACE_BEGIN namespace hatn { namespace acl {
#define HATN_ACL_NAMESPACE_END }}

#define HATN_ACL_NAMESPACE hatn::acl
#define HATN_ACL_NS acl
#define HATN_ACL_USING using namespace hatn::acl;

#endif // HATNACL_H
