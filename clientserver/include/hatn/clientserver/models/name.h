/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/name.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELNAME_H
#define HATNCLIENTSERVERMODELNAME_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(with_string,
    HDU_FIELD(string_value,TYPE_STRING,1)
)

HDU_UNIT(name,
    HDU_FIELD(first,TYPE_STRING,1)
    HDU_FIELD(middle,TYPE_STRING,2)
    HDU_FIELD(last,TYPE_STRING,3)
)

HDU_UNIT(with_name,
    HDU_FIELD(name,name::TYPE,71)
)

enum class NameFormat : uint8_t
{
    Full,
    LastwithInitials,
    LastFirst,
    FirstLast
};

class AsciiLocale
{
    public:

        auto firstLetter(lib::string_view word) const
        {
            if (word.empty())
            {
                return lib::string_view{};
            }
            return lib::string_view{word.data(),1};
        }
};

struct NameFormatter
{
    template <typename LocaleT=AsciiLocale>
    static std::string format(const name::type& obj, NameFormat nameFormat=NameFormat::FirstLast, const LocaleT& locale=LocaleT{})
    {
        auto first=obj.fieldValue(name::first);
        auto middle=obj.fieldValue(name::middle);
        auto last=obj.fieldValue(name::last);

        return format(first,middle,last,nameFormat,locale);
    }

    template <typename LocaleT=AsciiLocale>
    static std::string format(lib::string_view first, lib::string_view middle, lib::string_view last, NameFormat nameFormat=NameFormat::FirstLast, const LocaleT& locale=LocaleT{})
    {
        if (last.empty() && first.empty() && middle.empty())
        {
            return std::string{};
        }

        if (!last.empty())
        {
            if (!first.empty())
            {
                if (!middle.empty()) // non-empty all
                {
                    switch (nameFormat)
                    {
                    case (NameFormat::FirstLast):
                    {
                        return fmt::format("{} {}",first,last);
                    }
                    break;

                    case (NameFormat::LastFirst):
                    {
                        return fmt::format("{} {}",last,first);
                    }
                    break;

                    case (NameFormat::Full):
                    {
                        return fmt::format("{} {} {}",last,first,middle);
                    }
                    break;

                    case (NameFormat::LastwithInitials):
                    {
                        return fmt::format("{} {}.{}.",last,locale.firstLetter(first),locale.firstLetter(middle));
                    }
                    break;
                    }
                }
                else // non-empty last, non-empty first, empty middle
                {
                    switch (nameFormat)
                    {
                        case (NameFormat::FirstLast):
                        {
                            return fmt::format("{} {}",first,last);
                        }
                        break;

                        case (NameFormat::LastFirst):
                        {
                            return fmt::format("{} {}",last,first);
                        }
                        break;

                        case (NameFormat::Full):
                        {
                            return fmt::format("{} {}",last,middle);
                        }
                        break;

                        case (NameFormat::LastwithInitials):
                        {
                            return fmt::format("{} {}.",last,locale.firstLetter(first));
                        }
                        break;
                    }
                }
            }
            else // non-empty last, empty first
            {
                return std::string{last};
            }
        }
        else if (!first.empty())
        {
            if (!middle.empty()) // empty last, non-empty first, non-empty middle
            {
                return fmt::format("{} {}",first,middle);
            }
            else // empty last, non-empty first, empty middle
            {
                return std::string{first};
            }
        }
        else if (!middle.empty()) // empty last, empty first, non-empty middle
        {
            return std::string{middle};
        }

        // done
        return std::string{};
    }
};

struct formatNameT
{
    template <typename LocaleT=AsciiLocale>
    std::string operator()(const name::type& obj, NameFormat format=NameFormat::FirstLast, const LocaleT& locale=LocaleT{}) const
    {
        return NameFormatter::format(obj,format,locale);
    }
};
constexpr formatNameT formatName{};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELNAME_H
