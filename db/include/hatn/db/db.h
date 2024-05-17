/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/

/** @file db/db.h
  *
  *  Hatn Database Library implements types and routines to work with database using dataunit objects.
  *
  */

/****************************************************************************/

#ifndef HATNDB_H
#define HATNDB_H

#include <hatn/db/config.h>

// define export symbols for windows platform
#ifdef WIN32
#    ifndef HATN_DB_EXPORT
#        ifdef BUILD_HATN_DB
#            define HATN_DB_EXPORT __declspec(dllexport)
#        else
#            define HATN_DB_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define HATN_DB_EXPORT
#endif

#define HATN_DB_NAMESPACE_BEGIN namespace hatn { namespace db {
#define HATN_DB_NAMESPACE_END }}

#define HATN_DB_NAMESPACE hatn::db
#define HATN_DB_NS base
#define HATN_DB_USING using namespace hatn::db;

HATN_DB_NAMESPACE_BEGIN
constexpr const char* DB_MODULE_NAME="db";
HATN_DB_NAMESPACE_END

#endif // HATNDB_H
