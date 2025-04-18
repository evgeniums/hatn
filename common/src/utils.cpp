/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/utils.сpp
  *
  *      Utils.
  *
  */

#include <ctime>

#include <hatn/common/utils.h>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------
bool Utils::compareBoostAny(
        const boost::any& val1,
        const boost::any& val2
    ) noexcept
{
    const std::type_info& at = val1.type();
    const std::type_info& bt = val2.type();
    if (at!=bt)
    {
        return false;
    }

    if (at==typeid(int))
    {
        return boostAnyEqual<int>(val1,val2);
    }
    else if (at==typeid(bool))
    {
        return boostAnyEqual<bool>(val1,val2);
    }
    else if (at==typeid(std::string))
    {
        return boostAnyEqual<std::string>(val1,val2);
    }
    else if (at==typeid(double))
    {
        double precision = 1e-8;
        return (precision>std::abs(safeAny<double>(val1)-safeAny<double>(val2)));
    }
    else if (at==typeid(float))
    {
        double precision = 1e-8;
        return (precision>std::abs(safeAny<float>(val1)-safeAny<float>(val2)));
    }
    else if (at==typeid(unsigned int))
    {
        return boostAnyEqual<unsigned int>(val1,val2);
    }
    else if (at==typeid(char))
    {
        return boostAnyEqual<signed char>(val1,val2);
    }
    else if (at==typeid(unsigned char))
    {
        return boostAnyEqual<unsigned char>(val1,val2);
    }
    else if (at==typeid(short))
    {
        return boostAnyEqual<short>(val1,val2);
    }
    else if (at==typeid(unsigned short))
    {
        return boostAnyEqual<unsigned short>(val1,val2);
    }
    else if (at==typeid(long long))
    {
        return boostAnyEqual<long long>(val1,val2);
    }
    else if (at==typeid(unsigned long long))
    {
        return boostAnyEqual<unsigned long long>(val1,val2);
    }
    return false;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
