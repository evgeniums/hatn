/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/environment.h
 *
 *     Base class for environment classes.
 *
 */
/****************************************************************************/

#ifndef HATNENVIRONMENT_H
#define HATNENVIRONMENT_H

#include <hatn/common/common.h>
#include <hatn/common/interface.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Base class for environments
class Environment : public InterfacesContainer
{
    public:
};

//! Base class for Enviroment that can be aceessed via interfaces
template <typename ...Interfaces> using EnvironmentPack = common::Interfaces<Environment,Interfaces...>;

HATN_COMMON_NAMESPACE_END

#endif // HATNENVIRONMENT_H
