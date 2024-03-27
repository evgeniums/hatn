/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/unitstrings.h
  *
  *      Global object for localization of unit field names
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITSTRINGS_H
#define HATNDATAUNITSTRINGS_H

#include <map>
#include <string>

#include <hatn/common/singleton.h>

#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

class Unit;
class Field;

//! Global object for localization of dataunit field names.
class HATN_DATAUNIT_EXPORT UnitStrings : public common::Singleton
{
    public:

        HATN_SINGLETON_DECLARE()

        using common::Singleton::Singleton;

        /**
         * @brief Get name of dataunit's field.
         * @param unitName Name of dataunit.
         * @param fieldId ID of field to get name for.
         * @param fallbackName Use this value if name is not found.
         * @return Found field name or default field name.
         */
        std::string fieldName(const char* unitName, int fieldId, const char* fallbackName=nullptr) const;

        /**
         * @brief Get name of dataunit's field.
         * @param unit Dataunit.
         * @param fieldId ID of field to get name for.
         * @param fallbackName Use this value if name is not found.
         * @return Found field name or default field name.
         */
        std::string fieldName(const Unit* unit, int fieldId, const char* fallbackName=nullptr) const;

        /**
         * @brief Get name of dataunit's field.
         * @param field Field to get name for.
         * @return Found field name or default field name.
         */
        std::string fieldName(Field* field) const;

        /**
         * @brief Get name of dataunit's field.
         * @param UnitT Type of dataunit.
         * @param field Field to get name for.
         * @return Found field name or default field name.
         */
        template <typename UnitT, typename FieldT>
        std::string fieldName(const FieldT& field)
        {
            return fieldName(UnitT::unitName(),field.ID,field.name());
        }

        /**
         * @brief Register field name.
         * @param unitName Name of dataunit.
         * @param fieldId ID of field.
         * @param name Name to register for given field of given dataunit.
         */
        void registerFieldName(const char* unitName, int fieldId, std::string name);

        /**
         * @brief Register field name.
         * @param UnitT Type of dataunit.
         * @param fieldId ID of field.
         * @param name Name to register for given field of given dataunit.
         */
        template <typename UnitT, typename FieldT>
        void registerFieldName(const FieldT& field, std::string name)
        {
            registerFieldName(UnitT::unitName(),field.ID,std::move(name));
        }

        /**
         * @brief Check if no strings defined.
         * @return True if no strings defined.
         */
        bool isEmpty() const noexcept
        {
            return m_unitFieldNames.empty();
        }

        /**
         * @brief Free singleton's resources.
         */
        static void free();
        /**
         * @brief Get singleton's instance.
         * @return SIngleton's instance.
         */
        static UnitStrings& instance();

    private:

        std::string findField(const char *unitName, int fieldId) const;

        std::map<std::string,std::map<int,std::string>> m_unitFieldNames;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSTRINGS_H
