/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/objectwrapper.h
  */

/****************************************************************************/

#ifndef HATNOBJECTWRAPPER_H
#define HATNOBJECTWRAPPER_H

#include <hatn/common/allocatoronstack.h>
#include <hatn/db/modelsprovider.h>
#include <hatn/db/topic.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/systemsection.h>

HATN_UTILITY_NAMESPACE_BEGIN

constexpr const size_t ObjectNameLength=du::ObjectId::Length;

using ObjectType=common::StringOnStackT<ObjectNameLength>;

struct ObjectWrapper;

struct ObjectWrapperRef
{
    ObjectWrapperRef()=default;

    ObjectWrapperRef(
            lib::string_view id,
            lib::string_view topic,
            lib::string_view model={}
        ) : id(id),
            topic(topic),
            model(model)
    {}

    ObjectWrapperRef( const ObjectWrapper& ref);

    lib::string_view id;

    //! @todo Use TopicNameOrOidRef for topic
    db::Topic topic;
    lib::string_view model;
};

struct ObjectWrapper
{
    ObjectWrapper()=default;

    ObjectWrapper(
        lib::string_view id,
        lib::string_view topic,
        lib::string_view model={}
        ) : id(id),
            topic(topic),
            model(model)
    {}

    ObjectWrapper( const ObjectWrapperRef& ref)
        : id(ref.id),
          topic(ref.topic),
          model(ref.model)
    {}

    ObjectType id;
    //! @todo Use TopicNameOrOid for topic
    TopicType topic;
    lib::string_view model;
};

ObjectWrapperRef::ObjectWrapperRef( const ObjectWrapper& ref)
    : id(ref.id),
      topic(ref.topic),
      model(ref.model)
{}

using HierarchyItem = ObjectWrapperRef;

HATN_UTILITY_NAMESPACE_END

#endif // HATNOBJECTWRAPPER_H
