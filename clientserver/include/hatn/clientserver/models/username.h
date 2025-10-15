/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/username.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELUSERNAME_H
#define HATNCLIENTSERVERMODELUSERNAME_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/name.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const char* USERNAME_SCHEMA_HOST="host";

HDU_UNIT(username,
    HDU_FIELD(user,TYPE_STRING,1)
    HDU_FIELD(schema,TYPE_STRING,2)
    HDU_FIELD(domain,TYPE_STRING,3,false,USERNAME_SCHEMA_HOST)
)

struct formatUsernameT
{
    std::string operator()(const username::type& obj, const username::type& ref=username::type{}) const
    {
        if (obj.fieldValue(username::user).empty())
        {
            return std::string{};
        }

        if (obj.fieldValue(username::domain).empty() ||
                (
                    obj.fieldValue(username::domain)==ref.fieldValue(username::domain)
                    &&
                    obj.fieldValue(username::schema)==ref.fieldValue(username::schema)
                    &&
                    obj.fieldValue(username::schema)==USERNAME_SCHEMA_HOST
                )
            )
        {
            return fmt::format("@{}",obj.fieldValue(username::user));
        }

        return fmt::format("@{}@{}",obj.fieldValue(username::user),obj.fieldValue(username::domain));
    }
};
constexpr formatUsernameT formatUsername{};

HDU_UNIT(with_username,
    HDU_FIELD(uname,username::TYPE,70)
)

struct formatNameAndUsernameT
{
    template <typename ObjT, typename LocaleT=AsciiLocale>
    std::string operator()(const ObjT& obj, const username::type& ref, NameFormat nameFormat=NameFormat::FirstLast, const LocaleT& locale=LocaleT{}) const
    {
        if (obj.field(with_name::name).isSet())
        {
            auto str=formatName(obj.field(with_name::name).value(),nameFormat,locale);
            if (!str.empty())
            {
                return str;
            }
        }
        if (obj.field(with_username::uname).isSet())
        {
            return formatUsername(obj.field(with_username::uname).value(),ref);
        }

        return std::string{};
    }

    template <typename ObjT, typename LocaleT=AsciiLocale>
    std::string operator()(const ObjT& obj, NameFormat nameFormat=NameFormat::FirstLast, const LocaleT& locale=LocaleT{}) const;
};
constexpr formatNameAndUsernameT formatNameAndUsername{};

template <typename ObjT, typename LocaleT>
std::string formatNameAndUsernameT::operator()(const ObjT& obj, NameFormat nameFormat, const LocaleT& locale) const
{
    return formatNameAndUsername(obj,username::type{},nameFormat,locale);
}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELUSERNAME_H
