/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file section/section.h
  *
  */

/****************************************************************************/

#ifndef HATNSECTION_H
#define HATNSECTION_H

#include <hatn/section/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_SECTION_EXPORT
#   ifdef BUILD_HATN_SECTION
#       define HATN_SECTION_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_SECTION_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_SECTION_NAMESPACE_BEGIN namespace hatn { namespace section {
#define HATN_SECTION_NAMESPACE_END }}

#define HATN_SECTION_NAMESPACE hatn::section
#define HATN_SECTION_NS section
#define HATN_SECTION_USING using namespace hatn::section;

#endif // HATNSECTION_H
