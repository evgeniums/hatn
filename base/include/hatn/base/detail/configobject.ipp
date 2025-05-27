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

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/translate.h>
#include <hatn/common/runonscopeexit.h>

#include <hatn/validator/validate.hpp>

#include <hatn/dataunit/valuetypes.h>
#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/unitmeta.h>

#include <hatn/base/baseerrorcodes.h>
#include <hatn/base/configtree.h>
#include <hatn/base/configobject.h>

HATN_BASE_NAMESPACE_BEGIN

//---------------------------------------------------------------

namespace config_object_detail {

template <typename T, typename=hana::when<true>>
struct FieldTraits
{
    static Error set(const ConfigTree&,const ConfigTreePath&, T&)
    {
        return BaseError::UNSUPPORTED_TYPE;
    }
};

template <typename T, typename=hana::when<true>>
struct FieldValue
{
};

template <typename T>
struct FieldValue<T,hana::when<decltype(dataunit::types::IsBool<T::typeId>)::value>>
{
    using type=bool;
};

template <typename T>
struct FieldValue<T,hana::when<decltype(dataunit::types::IsScalarNotBool<T::typeId>)::value>>
{
    using type=typename T::type;
};

template <typename T>
struct FieldValue<T,hana::when<decltype(dataunit::types::IsString<T::typeId>)::value>>
{
    using type=std::string;
};

template <typename T>
struct FieldTraits<T,hana::when<!T::isRepeatedType::value>>
{
    using ValueT=typename FieldValue<T>::type;

    static Error set(const ConfigTree& t, const ConfigTreePath& path, T& field)
    {
        auto p=path.copyAppend(field.name());
        if (t.isSet(p,true))
        {
            auto val=t.get(p);
            if (val)
            {
                return val.takeError();
            }

            auto v=val->template as<ValueT>();
            if (v)
            {
                return v.takeError();
            }

            field.set(v.value());
        }

        return OK;
    }
};

template <typename T>
struct FieldTraits<T,hana::when<T::isRepeatedType::value>>
{
    using ValueT=typename FieldValue<T>::type;

    static Error set(const ConfigTree& t, const ConfigTreePath& path, T& field)
    {
        auto p=path.copyAppend(field.name());
        if (t.isSet(p,true))
        {
            field.clearArray();

            auto val=t.get(p);
            if (val)
            {
                return val.takeError();
            }

            auto arr=val->template asArray<ValueT>();
            if (arr)
            {
                return arr.takeError();
            }

            for (size_t i=0;i<arr->size();i++)
            {
                field.appendValue(arr->at(i));
            }
        }

        return OK;
    }
};

//! @todo Support map fields

} // namespace config_object_detail

//---------------------------------------------------------------

namespace detail {

template <typename T>
Error loadConfig(T& obj, const ConfigTree& configTree, const ConfigTreePath& path, bool checkRequired)
{
    auto requiredHandler=[&obj,&checkRequired,&path]()
    {
        // check required fields
        if (checkRequired)
        {
            auto reqResult=HATN_DATAUNIT_NAMESPACE::visitors::checkRequiredFields(obj);
            if (reqResult.first!=-1)
            {
                auto errMsg=fmt::format(_TR("required parameter \"{}\" not set","base"),reqResult.second);
                auto ec=std::make_shared<common::NativeError>(fmt::format(_TR("{} at path \"{}\": {}"), obj.name(), path.path(), errMsg));
                return baseError(BaseError::CONFIG_OBJECT_VALIDATE_ERROR,std::move(ec));
            }
        }
        return Error{};
    };

    // skip section if not set
    if (!configTree.isSet(path))
    {
        return requiredHandler();
    }

    // set each field
    std::string failedField;
    auto predicate=[](const Error& ec)
    {
        return !ec;
    };
    auto handler=[&configTree,&path,&failedField,checkRequired](auto& field, auto&&)
    {
        using fieldType=std::decay_t<decltype(field)>;

        constexpr auto isDataunit=HATN_DATAUNIT_NAMESPACE::types::IsDataunit<fieldType::typeId>;
        if constexpr (isDataunit.value)
        {
            if constexpr (fieldType::isRepeatedType::value)
            {
                auto p=path.copyAppend(field.name());
                if (configTree.isSet(p,true))
                {
                    auto arr=configTree.toArray<ConfigTree>(p);
                    if (arr)
                    {
                        return arr.takeError();
                    }
                    for (size_t i=0; i<arr->size(); i++)
                    {
                        auto& subfield=field.createAndAppendValue();
                        auto* mutableSubfield=subfield.mutableValue();
                        auto ec=loadConfig(*mutableSubfield,configTree,p.copyAppend(i),checkRequired);
                        HATN_CHECK_EC(ec)
                    }
                }
            }
            else
            {
                auto p=path.copyAppend(field.name());
                auto ec=loadConfig(*field.mutableValue(),configTree,p,checkRequired);
                HATN_CHECK_EC(ec)
            }

            return Error{};
        }
        else
        {
            auto ec=config_object_detail::FieldTraits<fieldType>::set(configTree,path,field);
            if (ec)
            {
                failedField=field.name();
            }
            return ec;
        }
    };
    auto ec=obj.each(predicate,handler);
    if (ec)
    {
        auto err=std::make_shared<common::NativeError>(fmt::format(_TR("{} at path \"{}\": parameter \"{}\"","base"), obj.name(),path.path(),failedField));
        err->setPrevError(std::move(ec));
        return baseError(BaseError::CONFIG_OBJECT_LOAD_ERROR,std::move(err));
    }

    // done
    return requiredHandler();
}

template <typename ObjT>
void fillLogRecords(const ObjT& obj, const config_object::LogSettings& logSettings, config_object::LogRecords& records, const std::string& parentPath="")
{
    auto handler=[&logSettings,&records,&parentPath](auto&& field, auto&&)
    {
        using T=typename std::decay_t<decltype(field)>;
        std::string path;
        if (parentPath.empty())
        {
            path=field.name();
        }
        else
        {
            path=fmt::format("{}.{}",parentPath,field.name());
        }

        auto value=hana::eval_if(
            typename T::isRepeatedType{},
            [&](auto)
            {
                return hana::eval_if(
                    dataunit::types::IsString<T::typeId>,
                    [&](auto _)
                    {
                        const auto& vector=_(field).value();
                        auto count=vector.size();
                        if (_(logSettings).CompactArrays && vector.size()>_(logSettings).MaxArrayElements)
                        {
                            count=_(logSettings).MaxArrayElements;
                        }

                        auto out = fmt::memory_buffer();
                        fmt::format_to(std::back_inserter(out),"[");
                        for (size_t i=0;i<count;i++)
                        {
                            if (i!=0)
                            {
                                fmt::format_to(std::back_inserter(out),",");
                            }

                            std::string v{_(field).at(i).c_str()};
                            _(logSettings).mask(_(field).name(),v);
                            fmt::format_to(std::back_inserter(out),"\"{}\"",v);
                        }

                        if (count!=vector.size())
                        {
                            fmt::format_to(std::back_inserter(out),",...]");
                        }
                        else
                        {
                            fmt::format_to(std::back_inserter(out),"]");
                        }
                        return fmt::to_string(out);
                    },
                    [&](auto _)
                    {
                        return hana::eval_if(
                            dataunit::types::IsDataunit<T::typeId>,
                            [&](auto _s)
                            {
                                for (size_t i=0;i<_s(_(field)).count();i++)
                                {
                                    auto nextPath=fmt::format("{}.{}",_s(_(path)),i);
                                    fillLogRecords(_s(_(field)).at(i).value(),_s(_(logSettings)),_s(_(records)),nextPath);
                                }
                                return std::string();
                            },
                            [&]( auto _s)
                            {
                                const auto& vector=_s(_(field)).value();
                                size_t endOffset{0};
                                if (_s(_(logSettings)).CompactArrays && vector.size()>_s(_(logSettings)).MaxArrayElements)
                                {
                                    endOffset=vector.size()-_s(_(logSettings)).MaxArrayElements;
                                }

                                auto out = fmt::memory_buffer();
                                fmt::format_to(std::back_inserter(out),"[{}",fmt::join(std::begin(vector),std::end(vector)-endOffset,","));
                                if (endOffset!=0)
                                {
                                    fmt::format_to(std::back_inserter(out),",...]");
                                }
                                else
                                {
                                    fmt::format_to(std::back_inserter(out),"]");
                                }
                                return fmt::to_string(out);
                            }
                        );
                    }
                );
            },
            [&](auto)
            {
                return hana::eval_if(
                    dataunit::types::IsString<T::typeId>,
                    [&](auto _)
                    {
                        std::string v{_(field).value()};
                        _(logSettings).mask(_(field).name(),v);
                        return fmt::format("{}",v);
                    },
                    [&](auto _)
                    {
                        return hana::eval_if(
                            dataunit::types::IsDataunit<T::typeId>,
                            [&](auto _s)
                            {
                                fillLogRecords(_s(_(field)).get(),_s(_(logSettings)),_s(_(records)),_s(_(path)));
                                return std::string();
                            },
                            [&](auto _)
                            {
                                return fmt::format("{}",_(field).value());
                            }
                        );
                    }
                );
            }
        );

        if (!value.empty())
        {
            records.emplace_back(path,std::move(value));
        }
        return true;
    };
    obj.each(HATN_DATAUNIT_NAMESPACE::meta::true_predicate,handler);
}

} // namespace detail

template <typename Traits>
Error ConfigObject<Traits>::loadConfig(const ConfigTree& configTree, const ConfigTreePath& path, bool checkRequired)
{
    return detail::loadConfig(_CFG,configTree,path,checkRequired);
}

//---------------------------------------------------------------

template <typename Traits>
template <typename ValidatorT>
Error ConfigObject<Traits>::loadConfig(const ConfigTree& configTree, const ConfigTreePath& path, const ValidatorT& validator)
{
    // load configuration
    auto ec=loadConfig(configTree,path);
    if (ec)
    {
        return ec;
    }

    // validate object
    return validate(path,validator);
}

//---------------------------------------------------------------

template <typename Traits>
template <typename ValidatorT>
Error ConfigObject<Traits>::validate(const ConfigTreePath& path, const ValidatorT& validator)
{
    HATN_VALIDATOR_NAMESPACE::error_report err;
    HATN_VALIDATOR_NAMESPACE::validate(_CFG,validator,err);
    if (err)
    {
        auto ec=std::make_shared<common::NativeError>(fmt::format(_TR("{} at path \"{}\": {}","base"), _CFG.name(), path.path(), err.message()));
        return baseError(BaseError::CONFIG_OBJECT_VALIDATE_ERROR,std::move(ec));
    }

    // done
    return OK;
}

//---------------------------------------------------------------

template <typename Traits>
Error ConfigObject<Traits>::loadLogConfig(const ConfigTree& configTree, const ConfigTreePath& path, config_object::LogRecords& records, const config_object::LogSettings& logSettings)
{
    auto onExit=common::makeScopeGuard(
        [this,&logSettings,&records,&path]()
        {
            fillLogRecords(logSettings,records,path.string());
        }
    );
    std::ignore=onExit;

    // load configuration
    auto ec=loadConfig(configTree,path);
    if (ec)
    {
        return ec;
    }

    // done
    return OK;
}

//---------------------------------------------------------------

template <typename Traits>
template <typename ValidatorT>
Error ConfigObject<Traits>::loadLogConfig(const ConfigTree& configTree, const ConfigTreePath& path, config_object::LogRecords& records, const ValidatorT& validator, const config_object::LogSettings& logSettings)
{
    auto onExit=common::makeScopeGuard(
        [this,&logSettings,&records,&path]()
        {
            fillLogRecords(logSettings,records,path.string());
        }
        );
    std::ignore=onExit;

    // load configuration
    auto ec=loadConfig(configTree,path);
    if (ec)
    {
        return ec;
    }

    // validate object
    return validate(path,validator);
}

//---------------------------------------------------------------

template <typename Traits>
void ConfigObject<Traits>::fillLogRecords(const config_object::LogSettings& logSettings, config_object::LogRecords& records, const std::string& parentLogPath)
{
    detail::fillLogRecords(_CFG,logSettings,records,parentLogPath);
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGOBJECT_IPP
