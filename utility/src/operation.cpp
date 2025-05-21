/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file utility/operation.сpp
  *
  */

#include <hatn/utility/operation.h>

HATN_UTILITY_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

Operation::~Operation()
{}

//--------------------------------------------------------------------------

std::string Operation::description(const common::Translator* translator) const
{
    std::ignore=translator;
    return m_name;
}

//--------------------------------------------------------------------------

HATN_UTILITY_NAMESPACE_END
