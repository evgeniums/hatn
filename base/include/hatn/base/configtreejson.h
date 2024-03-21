/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtreejson.h
  *
  * Contains declarations of ConfigTree JSON parser and serializer.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGTREEJSON_H
#define HATNCONFIGTREEJSON_H

#include <hatn/base/base.h>
#include <hatn/base/configtreeio.h>

HATN_BASE_NAMESPACE_BEGIN

/**
 * @brief ConfigTree parser and serializer in JSON format.
 * Implements "json" and "jsonc" formats. josonc stand for relaxed JSON with comments and trailing commas enabled.
 * Current implementation uses RapidJSON library.
 */
class HATN_BASE_EXPORT ConfigTreeJson : public ConfigTreeIo
{
    public:

        /**
         * @brief Constructor.
         */
        ConfigTreeJson():ConfigTreeIo({"json","jsonc"})
        {}

        /**
         * @brief Load config tree from text.
         * @param target Target config tree.
         * @param source Source text.
         * @param root Root node where to merge parsed tree to.
         * @param format Source format, if empty then either autodetect format or use default format of the loader.
         * @return Operation status.
         */
        virtual Error doParse(
            ConfigTree& target,
            const common::lib::string_view& source,
            const ConfigTreePath& root=ConfigTreePath(),
            const std::string& format=std::string()
        ) const noexcept override;

        /**
         * @brief Serialize config tree to text.
         * @param source Config tree.
         * @param root Root node to serialize from.
         * @param format Target format, if empty then use default format of the loader.
         * @return Operation result with string containing serialized configuration.
         */
        virtual Result<std::string> doSerialize(
            const ConfigTree& source,
            const ConfigTreePath& root=ConfigTreePath(),
            const std::string& format=std::string()
        ) const noexcept override;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREEJSON_H
