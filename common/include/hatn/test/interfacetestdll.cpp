/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file interfacetestdll.сpp
  *
  *  Source to test interfaces use when crossing DLL packages.
  *
  */

#include <iostream>
#include <hatn/test/interfacetestdll.h>

HATN_COMMON_NAMESPACE_BEGIN

#ifdef TEST_INTERFACE_DLL
HATN_CUID_INIT_MULTI(Johny)
HATN_CUID_INIT(Johny1)
HATN_CUID_INIT(Johny2)
HATN_CUID_INIT(Johny3)

Johny::Johny(
    ) : id1(Johny1::cuid()),
        id2(Johny2::cuid()),
        id3(Johny3::cuid()),
        id4(Johny::cuid()),
        ids(Johny::cuids())
{
#ifdef TEST_INTERFACE_DLL_PRINT

    std::cerr<<"Johny cuids: ";
    for (auto&& it: Johny::cuids())
    {
        std::cerr<<"0x"<<std::hex<<it<<",";
    }
    std::cerr<<std::endl;

    std::cerr<<"Johny1 "<<"0x"<<std::hex<<Johny1::cuid()<<std::endl;
    std::cerr<<"Johny2 "<<"0x"<<std::hex<<Johny2::cuid()<<std::endl;
    std::cerr<<"Johny3 "<<"0x"<<std::hex<<Johny3::cuid()<<std::endl;
    std::cerr<<"Johny "<<"0x"<<std::hex<<Johny::cuid()<<std::endl;

#endif
}
#endif

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
