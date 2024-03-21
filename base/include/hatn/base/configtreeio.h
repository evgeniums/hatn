/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtreeio.h
  *
  * Contains declaration of base class configuration tree IO classes.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGTREELOADER_H
#define HATNCONFIGTREELOADER_H

#include <set>

#include <hatn/common/error.h>

#include <hatn/base/base.h>
#include <hatn/base/baseerror.h>
#include <hatn/base/configtree.h>

HATN_COMMON_NAMESPACE_BEGIN

class File;

HATN_COMMON_NAMESPACE_END

HATN_BASE_NAMESPACE_BEGIN

/**
 * @brief Error of config tree configuration parsing.
 */
class HATN_BASE_EXPORT ConfigTreeParseError : public common::NativeError
{
    public:

        ConfigTreeParseError(
                std::string message,
                size_t offset=0,
                int nativeCode=-1
            ) : NativeError(std::move(message),nativeCode,&BaseErrorCategory::getCategory()),
                m_offset(offset)
        {}

        size_t offset() const noexcept
        {
            return m_offset;
        }

    private:

        size_t m_offset;
};

/**
 * @brief Base class for readers and writers of config tree configurations.
 */
class HATN_BASE_EXPORT ConfigTreeIo
{
    public:

        /**
         * @brief Load config tree from text.
         * @param target Target config tree.
         * @param source Source text.
         * @param root Root node where to merge parsed tree to.
         * @param format Source format, if empty then either autodetect format or use default format of the loader.
         * @return Operation status.
         */
        Error parse(
            ConfigTree& target,
            common::lib::string_view source,
            const ConfigTreePath& root=ConfigTreePath(),
            const std::string& format=std::string()
        ) const noexcept
        {
            if (!format.empty() && !supportsFormat(format))
            {
                return baseError(BaseError::UNSUPPORTED_CONFIG_FORMAT);
            }
            return doParse(target,source,root,format);
        }

        /**
         * @brief Serialize config tree to text.
         * @param source Config tree.
         * @param format Target format, if empty then use default format of the loader.
         * @return Operation result with string containing serialized configuration.
         */
        Result<std::string> serialize(
            const ConfigTree& source,
            const ConfigTreePath& root=ConfigTreePath(),
            const std::string& format=std::string()
        ) const noexcept
        {
            if (!format.empty() && !supportsFormat(format))
            {
                return baseError(BaseError::UNSUPPORTED_CONFIG_FORMAT);
            }
            return doSerialize(source,root,format);
        }

        ConfigTreeIo(ConfigTreeIo&&)=default;
        ConfigTreeIo& operator =(ConfigTreeIo&&)=default;

        virtual ~ConfigTreeIo();

        ConfigTreeIo(const ConfigTree&)=delete;
        ConfigTreeIo& operator =(const ConfigTree&)=delete;

        bool supportsFormat(const std::string& format) const noexcept
        {
            return m_formats.find(format)!=m_formats.end();
        }

        const std::set<std::string>& formats() const noexcept
        {
            return m_formats;
        }

        static std::string fileFormat(lib::string_view filename);

        Error loadFile(
            ConfigTree& target,
            common::File& file,
            const ConfigTreePath& root=ConfigTreePath(),
            const std::string& format=std::string()
        );

        Error loadFile(
            ConfigTree& target,
            lib::string_view filename,
            const ConfigTreePath& root=ConfigTreePath(),
            const std::string& format=std::string()
        );

    protected:

        /**
         * @brief Protected destructo must be called by derived classes.
         * @param formats List formats supported by this loader, e.g. {"json","jsonc"}.
         */
        ConfigTreeIo(std::set<std::string> formats):m_formats(std::move(formats))
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
            ) const noexcept =0;

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
            ) const noexcept =0;

    private:

        std::set<std::string> m_formats;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREELOADER_H
