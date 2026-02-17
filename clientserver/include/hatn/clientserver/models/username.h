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

#include <hatn/common/utils.h>
#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/name.h>
#include <hatn/clientserver/models/oid.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const char* USERNAME_SCHEMA_HOST="host";

HDU_UNIT(uri,
    HDU_FIELD(user,TYPE_STRING,1)
    HDU_FIELD(schema,TYPE_STRING,2)
    HDU_FIELD(domain,TYPE_STRING,3)
)

HDU_UNIT(with_uri,
    HDU_FIELD(uname,uri::TYPE,70)
)

namespace username=uri;
namespace with_username=with_uri;

HDU_UNIT_WITH(username_reference,(HDU_BASE(HATN_DB_NAMESPACE::object),HDU_BASE(with_username)),
    HDU_FIELD(reference,topic_object::TYPE,1)
)

struct formatUsernameT
{
    std::string operator()(const username::type& obj, const username::type& ref=username::type{}, bool forName=false) const
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
            if (forName)
            {
               return  std::string{obj.fieldValue(username::user)};
            }
            else
            {
                return fmt::format("@{}",obj.fieldValue(username::user));
            }
        }

        if (forName)
        {
            return fmt::format("{}@{}",obj.fieldValue(username::user),obj.fieldValue(username::domain));
        }
        return fmt::format("@{}@{}",obj.fieldValue(username::user),obj.fieldValue(username::domain));
    }
};
constexpr formatUsernameT formatUsername{};

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
            return formatUsername(obj.field(with_username::uname).value(),ref,true);
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

struct parseUsernameT
{
    template <typename T>
    auto operator()(T&& str) const
    {
        std::string text{str};

        auto uname=common::makeShared<username::managed>();
        std::vector<std::string> parts;
        common::Utils::trimSplit(parts,text,'@');
        if (parts.size()==1)
        {
            uname->setFieldValue(username::user,parts[0]);
        }
        else if (parts.size()==2)
        {
            if (parts[0].empty())
            {
                uname->setFieldValue(username::user,parts[1]);
            }
            else
            {
                uname->setFieldValue(username::user,parts[0]);
                uname->setFieldValue(username::domain,parts[1]);
            }
        }
        else if (parts.size()==3)
        {
            uname->setFieldValue(username::user,parts[1]);
            uname->setFieldValue(username::domain,parts[2]);
        }
        return uname;
    }
};
constexpr parseUsernameT parseUsername{};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELUSERNAME_H
