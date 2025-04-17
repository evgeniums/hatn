/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configobject.h
  *
  * Contains declarations of configurable object.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGOBJECT_H
#define HATNCONFIGOBJECT_H

#include <vector>
#include <set>

#include <hatn/common/error.h>
#include <hatn/common/containerutils.h>
#include <hatn/common/result.h>

#include <hatn/dataunit/unit.h>

#include <hatn/base/base.h>
#include <hatn/base/configtreepath.h>

HATN_BASE_NAMESPACE_BEGIN

class ConfigTree;

namespace config_object {

struct LogRecord
{
    std::string name;
    std::string value;

    LogRecord(std::string name,std::string value)
        : name(std::move(name)),value(std::move(value))
    {}

    LogRecord()
    {}

    std::string string()
    {
        return fmt::format("\"{}\": {}", name, value);
    }
};
using LogRecords=std::vector<LogRecord>;

struct LogParams
{
    bool CompactArrays=true;
    size_t MaxArrayElements=10;
    const char* Mask="********";
};

struct LogSettings : public LogParams
{
    std::set<std::string> MaskNames;

    LogSettings(const LogParams& params=LogParams())
        : LogParams(params),
          MaskNames{{"password","secret","masked","passphrase","passwd"}}
    {}

    template <typename ...Args>
    LogSettings(Args&& ...patterns)
    {
        common::ContainerUtils::insert(MaskNames,std::forward<Args>(patterns)...);
    }

    void mask(const std::string name, std::string& value) const
    {
        if (MaskNames.find(name)!=MaskNames.end())
        {
            value=Mask;
        }
    }
};

} // namespace config_object

template <typename Traits>
class ConfigObject
{
    public:

        static_assert(std::is_base_of<dataunit::Unit,Traits>::value,"Config traits must be derived from dataunit::Unit");

        Error loadConfig(const ConfigTree& configTree, const ConfigTreePath& path, bool checkRequired=true);

        template <typename ValidatorT>
        Error loadConfig(const ConfigTree& configTree, const ConfigTreePath& path, const ValidatorT& validator);

        template <typename ValidatorT>
        Error loadLogConfig(const ConfigTree& configTree,  const ConfigTreePath& path, config_object::LogRecords& records, const ValidatorT& validator, const config_object::LogSettings& logSettings=config_object::LogSettings());

        Error loadLogConfig(const ConfigTree& configTree, const ConfigTreePath& path, config_object::LogRecords& records, const config_object::LogSettings& logSettings=config_object::LogSettings());

        const Traits& config() const
        {
            return _CFG;
        }

        Traits& config()
        {
            return _CFG;
        }

    protected:

        Traits _CFG;

    private:

        void fillLogRecords(const config_object::LogSettings& logSettings, config_object::LogRecords& records, const std::string& parentLogPath="");

        template <typename ValidatorT>
        Error validate(const ConfigTreePath& path, const ValidatorT& validator);
};

HATN_BASE_NAMESPACE_END

#include <hatn/base/detail/configobject.ipp>

#endif // HATNCONFIGOBJECT_H
