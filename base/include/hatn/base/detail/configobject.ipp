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
    static Error set(const config_tree::MapT&, T&)
    {
        // static_assert(false,"Invalid operation for this field type");
        return OK;
    }
};

template <typename T>
struct FieldTraits<T,hana::when<decltype(dataunit::types::IsScalar<T::typeId>)::value>>
{
    static Error set(const ConfigTree& t, T& field)
    {
        if (t.isSet(field.name()))
        {
            auto val=t.get(field.name());
            HATN_CHECK_RESULT(val)
            auto scalar=val->template as<typename T::type>();
            HATN_CHECK_RESULT(scalar)
            field.set(scalar.value());
        }

        return OK;
    }
};

} // namespace config_object_detail

//---------------------------------------------------------------

template <typename Traits>
Error ConfigObject<Traits>::loadConfig(const ConfigTree& configTree, const ConfigTreePath& path)
{
    auto cfg=configTree.get(path);
    if (cfg)
    {
        //! @todo make native error with message
        return cfg.error();
    }

    Error ec;
    auto predicate=[](const Error& ec)
    {
        return !ec;
    };
    auto handler=[&cfg](auto& field, auto&&)
    {
        return config_object_detail::FieldTraits<std::decay_t<decltype(field)>>::set(cfg.value(),field);
    };

    auto r=_CFG.each(predicate,handler);
    ec=r;
    return ec;
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGOBJECT_IPP
