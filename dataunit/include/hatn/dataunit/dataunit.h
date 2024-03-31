/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/dataunit.h
  *
  *  Hatn DataUnit Library for units definition and serialization/deserialization
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNIT_H
#define HATNDATAUNIT_H

#include <hatn/common/logger.h>
#include <hatn/dataunit/config.h>

// define export symbols for windows platform
#ifdef WIN32
#    ifndef HATN_DATAUNIT_EXPORT
#        ifdef BUILD_HATN_DATAUNIT
#            define HATN_DATAUNIT_EXPORT __declspec(dllexport)
#        else
#            define HATN_DATAUNIT_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define HATN_DATAUNIT_EXPORT
#endif

#define HATN_DATAUNIT_NAMESPACE_BEGIN namespace hatn { namespace dataunit {
#define HATN_DATAUNIT_NAMESPACE_END }}

#define HATN_DATAUNIT_NAMESPACE hatn::dataunit
#define HATN_DATAUNIT_NS dataunit
#define HATN_DATAUNIT_USING using namespace hatn::dataunit;

DECLARE_LOG_MODULE_EXPORT(dataunit,HATN_DATAUNIT_EXPORT)

#endif // HATNDATAUNIT_H
