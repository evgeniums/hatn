/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file callgraph/callgraph.h
  *
  *  Hatn CallGraph Library contains types and helpers to buld call graphs
  *  and log call stacks.
  *
  */

/****************************************************************************/

#ifndef HATNCALLGRAPH_H
#define HATNCALLGRAPH_H

#include <hatn/callgraph/config.h>

// define export symbols for windows platform
#ifdef WIN32
#    ifndef HATN_CALLGRAPH_EXPORT
#        ifdef BUILD_HATN_CALLGRAPH
#            define HATN_CALLGRAPH_EXPORT __declspec(dllexport)
#        else
#            define HATN_CALLGRAPH_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define HATN_CALLGRAPH_EXPORT
#endif

#define HATN_CALLGRAPH_NAMESPACE_BEGIN namespace hatn { namespace callgraph {
#define HATN_CALLGRAPH_NAMESPACE_END }}

#define HATN_CALLGRAPH_NAMESPACE hatn::callgraph
#define HATN_CALLGRAPH_NS callgraph
#define HATN_CALLGRAPH_USING using namespace hatn::callgraph;

#endif // HATNCALLGRAPH_H
