/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/detail/configobject.ipp
  *
  * Contains definitions of ConfigObject template methods.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGOBJECT_IPP
#define HATNCONFIGOBJECT_IPP

#include <hatn/dataunit/valuetypes.h>
#include <hatn/dataunit/unit.h>

#include <hatn/base/configtree.h>
#include <hatn/base/configobject.h>

HATN_BASE_NAMESPACE_BEGIN

//---------------------------------------------------------------

namespace config_object_detail {

template <typename T, typename=hana::when<true>>
struct FieldTraits
{
    static Error set(const ConfigTree&,const ConfigTreePath&,  T&)
    {
        return BaseError::INVALID_TYPE;
    }
};

template <typename T,typename ValueT>
Error setValue(const ConfigTree& t, const ConfigTreePath& path, T& field)
{
    //! @todo construct error with path

    auto p=path.copyAppend(field.name());
    if (t.isSet(p,true))
    {
        auto val=t.get(p);
        HATN_CHECK_RESULT(val)
        auto scalar=val->template as<ValueT>();
        HATN_CHECK_RESULT(scalar)
        field.set(scalar.value());
    }

    return OK;
}

template <typename T>
struct FieldTraits<T,hana::when<decltype(dataunit::types::IsScalar<T::typeId>)::value>>
{
    static Error set(const ConfigTree& t, const ConfigTreePath& path, T& field)
    {
        return setValue<T,typename T::type>(t,path,field);
    }
};

template <typename T>
struct FieldTraits<T,hana::when<decltype(dataunit::types::IsString<T::typeId>)::value>>
{
    static Error set(const ConfigTree& t, const ConfigTreePath& path, T& field)
    {
        return setValue<T,std::string>(t,path,field);
    }
};

} // namespace config_object_detail

//---------------------------------------------------------------

template <typename Traits>
Error ConfigObject<Traits>::loadConfig(const ConfigTree& configTree, const ConfigTreePath& path)
{
    // skip section if not set
    if (!configTree.isSet(path))
    {
        return OK;
    }

    // set each field
    auto predicate=[](const Error& ec)
    {
        return !ec;
    };
    auto handler=[&configTree,&path](auto& field, auto&&)
    {
        return config_object_detail::FieldTraits<std::decay_t<decltype(field)>>::set(configTree,path,field);
    };
    auto r=_CFG.each(predicate,handler);
    return r;
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGOBJECT_IPP
