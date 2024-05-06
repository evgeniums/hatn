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

struct HATN_BASE_EXPORT Record
{
    ConfigTreePath path;
    std::string value;
};
using Records=std::vector<Record>;

struct HATN_BASE_EXPORT LogParams
{
    bool CompactArrays=true;
    size_t MaxArrayElements=10;
    const char* Mask="********";
};

struct HATN_BASE_EXPORT LogSettings : public LogParams
{
    std::set<std::string> MaskPatterns;

    LogSettings(const LogParams& params=LogParams())
        : LogParams(params),
          MaskPatterns{{"password","secret","masked"}}
    {}

    template <typename ...Args>
    LogSettings(Args&& ...patterns)
    {
        common::ContainerUtils::insertElements(MaskPatterns,std::forward<Args>(patterns)...);
    }
};

template <typename KeyT, typename ValueT>
static Error loadConfigMap(const ConfigTree& configTree, const ConfigTreePath& path, std::map<KeyT,ValueT>& map);

} // namespace config_object

template <typename Traits>
class ConfigObject
{
    public:

        static_assert(std::is_base_of<dataunit::Unit,Traits>::value,"Config traits must be derived from dataunit::Unit");

        Error loadConfig(const ConfigTree& configTree, const ConfigTreePath& path);

        template <typename ValidatorT>
        Error loadConfig(const ConfigTree& configTree, const ConfigTreePath& path, const ValidatorT& validator);

        Result<config_object::Records> loadLogConfig(const ConfigTree& configTree, const ConfigTreePath& path, const config_object::LogSettings& logSettings=config_object::LogSettings());

        template <typename ValidatorT>
        Result<config_object::Records> loadLogConfig(const ConfigTree& configTree,  const ConfigTreePath& path, const ValidatorT& validator, const config_object::LogSettings& logSettings=config_object::LogSettings());

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
};

HATN_BASE_NAMESPACE_END

#include <hatn/base/detail/configobject.ipp>

#endif // HATNCONFIGOBJECT_H
