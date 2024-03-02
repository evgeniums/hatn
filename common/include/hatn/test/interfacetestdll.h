/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/*
    
*/
/** @file interfacetestdll.h
 *
 *     Header to test interfaces use when crossing DLL packages.
 *
 */
/****************************************************************************/

#ifndef HATNINTERFACETESTDLL_H
#define HATNINTERFACETESTDLL_H

#include <hatn/common/interface.h>

HATN_COMMON_NAMESPACE_BEGIN

#define TEST_INTERFACE_DLL
//#define TEST_INTERFACE_DLL_PRINT

#ifdef TEST_INTERFACE_DLL
class HATN_COMMON_EXPORT Johny1 : public Interface<Johny1>
{
    public:
        HATN_CUID_DECLARE()

        int a;
        double b;
};
class HATN_COMMON_EXPORT Johny2 : public Interface<Johny2>
{
    public:
        HATN_CUID_DECLARE()

        char c[100];
};
class HATN_COMMON_EXPORT Johny3 : public Interface<Johny3>
{
    public:
        HATN_CUID_DECLARE()

        char k;
        char l;
};
class HATN_COMMON_EXPORT Johny : public MultiInterface<Johny,Johny1,Johny2,Johny3>
{
    public:

        CUID_TYPE id1;
        CUID_TYPE id2;
        CUID_TYPE id3;
        CUID_TYPE id4;
        std::set<CUID_TYPE> ids;

        Johny();

        HATN_CUID_DECLARE_MULTI()
};
#endif

HATN_COMMON_NAMESPACE_END

#endif // HATNINTERFACETESTDLL_H
