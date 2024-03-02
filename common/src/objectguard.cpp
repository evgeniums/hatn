/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/objectguard.cpp
 *
 *     Guarded object that watches object existence
 *
 */

#include <hatn/common/objectguard.h>

HATN_COMMON_NAMESPACE_BEGIN

pmr::memory_resource* WithGuard::m_memoryResource=nullptr;

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
