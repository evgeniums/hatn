/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/unitstrings.cpp
  *
  *      Global object for localization of unit field names
  *
  */

#include <hatn/dataunit/field.h>
#include <hatn/dataunit/unit.h>

#include <hatn/dataunit/unitstrings.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

HATN_SINGLETON_INIT(UnitStrings)

/********************** UnitStrings **************************/

//---------------------------------------------------------------
void UnitStrings::free()
{
    instance().m_unitFieldNames.clear();
}

//---------------------------------------------------------------
UnitStrings& UnitStrings::instance()
{
    static UnitStrings Instance;
    return Instance;
}

//---------------------------------------------------------------
std::string UnitStrings::findField(const char *unitName, int fieldId) const
{
    auto it1=m_unitFieldNames.find(std::string(unitName));
    if (it1!=m_unitFieldNames.end())
    {
        const auto& unitFields=it1->second;
        auto it2=unitFields.find(fieldId);
        if (it2!=unitFields.end())
        {
            return it2->second;
        }
    }
    return std::string();
}

//---------------------------------------------------------------
std::string UnitStrings::fieldName(const char *unitName, int fieldId, const char* fallbackName) const
{
    auto str=findField(unitName,fieldId);
    if (!str.empty())
    {
        return str;
    }
    if (fallbackName!=nullptr)
    {
        return std::string(fallbackName);
    }
    return std::to_string(fieldId);
}

//---------------------------------------------------------------
std::string UnitStrings::fieldName(const Unit *unit, int fieldId, const char *fallbackName) const
{
    return fieldName(unit->name(),fieldId,fallbackName);
}

//---------------------------------------------------------------
std::string UnitStrings::fieldName(Field* field) const
{
    return fieldName(field->unit(),field->getID(),field->name());
}

//---------------------------------------------------------------
void UnitStrings::registerFieldName(const char *unitName, int fieldId, std::string name)
{
    std::string unitNameStr(unitName);
    std::map<int,std::string>* unitFields=nullptr;
    auto unitInsetsion=m_unitFieldNames.emplace(std::make_pair(unitNameStr,std::map<int,std::string>()));
    unitFields=&unitInsetsion.first->second;
    (*unitFields)[fieldId]=std::move(name);
}

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END
