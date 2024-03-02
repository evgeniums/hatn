/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
*/

/****************************************************************************/
/*
    
*/
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
