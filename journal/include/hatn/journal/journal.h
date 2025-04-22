/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file journal/journal.h
  *
  */

/****************************************************************************/

#ifndef HATNJOURNAL_H
#define HATNJOURNAL_H

#include <hatn/journal/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_JOURNAL_EXPORT
#   ifdef BUILD_HATN_JOURNAL
#       define HATN_JOURNAL_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_JOURNAL_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_JOURNAL_NAMESPACE_BEGIN namespace hatn { namespace journal {
#define HATN_JOURNAL_NAMESPACE_END }}

#define HATN_JOURNAL_NAMESPACE hatn::journal
#define HATN_JOURNAL_NS journal
#define HATN_JOURNAL_USING using namespace hatn::journal;

#endif // HATNJOURNAL_H
