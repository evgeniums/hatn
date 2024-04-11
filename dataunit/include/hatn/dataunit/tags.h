/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/tags.h
  *
  *  Contains tags templates and macros.
  *
  */

/****************************************************************************/

#ifndef HATNTAGS_H
#define HATNTAGS_H

#include <boost/hana.hpp>
namespace hana=boost::hana;

#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

#define HDU_TAG_DECLARE(Name)\
    constexpr const char* tag_v_##Name=#Name;\
    struct tag_t_##Name {constexpr static const auto& value=tag_v_##Name;};\
    constexpr tag_t_##Name tag_##Name{};

#define HDU_TAG_NAME(Name) std::decay_t<decltype(tag_##Name)>::value

#define HDU_TAG_ID(Name) hana::type_c<std::decay_t<decltype(tag_##Name)>>

#define HDU_TAG(Name,Value) hana::make_pair(HDU_TAG_ID(Name),Value)

#define HDU_TAG_VALUE(Tag) hana::second(Tag)

#define HDU_TAG_KEY(Tag) hana::first(Tag)

#define HDU_TAG_KEY_NAME(Tag) std::decay_t<decltype(HDU_TAG_KEY(Tag))>::type::value

struct tags_group_t
{
    template <typename ...Tags>
    auto operator ()(Tags&& ...tags) const
    {
        return hana::make_map(std::forward<Tags>(tags)...);
    }
};
constexpr tags_group_t tags_group{};

struct field_tags_t
{
    template <typename FieldT, typename TagsGroup>
    auto operator ()(FieldT&&, TagsGroup&& tags) const
    {
        return hana::eval_if(
            hana::is_a<hana::map_tag>(tags),
            [&](auto _)
            {
                return hana::make_pair(hana::type_c<std::decay_t<FieldT>>,_(tags));
            },
            [&](auto _)
            {
                static_assert(decltype(hana::is_a<hana::pair_tag,TagsGroup>)::value,"");
                return hana::make_pair(hana::type_c<std::decay_t<FieldT>>,tags_group(_(tags)));
            }
        );
    }

    template <typename FieldT, typename ...Tags>
    auto operator ()(FieldT&&, Tags&& ...tags) const
    {
        return hana::make_pair(hana::type_c<std::decay_t<FieldT>>,tags_group(std::forward<Tags>(tags)...));
    }
};
constexpr field_tags_t field_tags{};

#define HDU_EXTRACT_FIELD_TAG(FieldTags,Key) hana::at_key(FieldTags,HDU_TAG_ID(Key))

struct unit_field_tags_t
{
    template <typename ...FieldTags>
    auto operator ()(FieldTags&& ...fieldTags) const
    {
        return hana::make_map(std::forward<FieldTags>(fieldTags)...);
    }
};
constexpr unit_field_tags_t unit_field_tags{};

#define HDU_FIELD_TAG(UnitTags,Field,Key) HDU_EXTRACT_FIELD_TAG(hana::at_key(UnitTags,hana::type_c<std::decay_t<decltype(Field)>>),Key)

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNTAGS_H
