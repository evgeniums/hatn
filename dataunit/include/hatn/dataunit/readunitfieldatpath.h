/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/readunitfieldatpath.h
  *
  *  Defines helpers for reading values of data unit fields at certain path.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITREADUNITFIELDATPATH_H
#define HATNDATAUNITREADUNITFIELDATPATH_H

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/updateunitfieldatpath.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/**
 * @brief Helper for getting value of dataunit's field at some path.
 * @param unit Dataunit object for lookup.
 * @param path Nested field path.
 * @return Value of the nested field at the given path.
 */
struct getUnitFieldAtPathT
{
    template <typename UnitT, typename PathT>
    auto operator () (const UnitT& unit, const PathT& path) const -> decltype(auto)
    {
        return HATN_VALIDATOR_NAMESPACE::get_member(unit,path.path());
    }
};
/** Callable for getting value of dataunit's field at some path. **/
constexpr getUnitFieldAtPathT getUnitFieldAtPath{};

/**
 * @brief Helper for getting size of dataunit's field content at some path.
 * @param unit Dataunit object for lookup.
 * @param path Nested field path.
 * @return Size of the nested field's content at the given path.
 */
struct sizeUnitFieldAtPathT
{
    template <typename UnitT, typename PathT>
    size_t operator () (const UnitT& unit, const PathT& path) const
    {
        return UnitFieldUpdater::fieldAtPath(unit,path).dataSize();
    }
};
/** Callable for getting size of dataunit's field's content at some path. **/
constexpr sizeUnitFieldAtPathT sizeUnitFieldAtPath{};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITREADUNITFIELDATPATH_H
