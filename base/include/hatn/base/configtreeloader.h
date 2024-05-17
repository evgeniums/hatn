/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtreeloader.h
  *
  * Contains declaration of ConfigTree loader.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGTREELOADER_H
#define HATNCONFIGTREELOADER_H

#include <hatn/common/containerutils.h>

#include <hatn/base/base.h>
#include <hatn/base/configtreeio.h>

HATN_BASE_NAMESPACE_BEGIN

/**
 * @brief Descriptor of include file.
 */
struct ConfigTreeInclude
{
    ConfigTreeInclude()=default;

    ConfigTreeInclude(std::string file):file(std::move(file))
    {}

    static Result<ConfigTreeInclude> fromMap(const config_tree::MapT&);

    /**
     * @brief File path.
     *
     * File path can be either absolute or relative.
     * In case of relative path the file will be looked in the folder where original configuration file resides.
     * If not found then the list of include directories will be used for file lookup.
     *
     * If file path is empty then this descriptor is used only to figure out the merge mode
     * for merging current config into pre-merged list of includes.
     */
    std::string file;

    /**
     * @brief Mode of arrays merging. Can be one of: "merge", "append", "prepend", "override", "preserve".
     *
     * @note This mode affetcs only merging of one include file into another provided that both includes are of the same list of the same config.
     * In general, the final merging of the current config does not respect this mode becase it is merged into result of previous merges where the modes are already mixed.
     * To set the mode of the the final merging of the current config add a descriptor with empty file name to the list of includes.
     */
    config_tree::ArrayMerge merge=config_tree::ArrayMerge::Merge;

    /**
     * @brief If true then merge direction is reversed.
     */
    bool extend=false;

    /**
     * @brief Format of inculde file. If empty then autodetect by file name extension.
     */
    std::string format;
};

/**
 * @brief The ConfigTreeLoader class implements methods for loading a configuration tree from text files and for saving a configuration tree to a text file.
 *
 * Configuration files can include other configuration files using include tag as a map key at any path level.
 * A value mapped to the include tag key is an array of either file names or ConfigTreeInclude objects. If it is a plain array of file names then default settings of ConfigTreeInclude are applied.
 *
 * There are a few modes of including:
 *
 * 1. If include tag is located at the top level of current configuration tree then all include files are parsed into respective config tree objects one by one sequentially so that each successive config tree is merged into preceeding config tree.
 *    Current config tree is merged at the final step into the result of merging of the list of all included files. In other words, each next include file and at the end the current configuation file would possibly
 *    override parameters retrieved from the previously included files.
 *
 * 2. Order of merging can be reversed if ConfigTreeInclude.extand=true. In this case the included config tree will be merged into current config tree, thus possibly overriding parameters of the current config tree.
 *
 * 3. If include tag is located somewhere on the nested path in the config tree then it will be merged at that path into the current config tree regardless of the ConfigTreeInclude.expand flag.
 *
 * Files are included recursively, i.e. each included file can in turn include other files and so on.
 *
 * The name of an include file can be presented either as an absolute path or as a relative path.
 * If the name is a relative path then, at first, the loader tries to find the include file in the same directory with the current config file and then, if not found,
 * it uses a list of include directories to look for the file.
 *
 * Configuration files can contain other parameters containing some relative file paths, in addition to the includes. If relFilePathPrefix() is not empty then each string parameter in configuration file
 * that starts with relFilePathPrefix() will be expanded to absolute path by means of replacing relFilePathPrefix() with the absolute path of the root configuration file.
 */
class HATN_BASE_EXPORT ConfigTreeLoader
{
    public:

        static std::string DefaultFormat; // "jsonc"
        static std::string DefaultIncludeTag; // "#include"
        static std::string DefaultPathSeparator; // "."
        static std::string DefaultRelFilePathPrefix; // "$rel/"

        ConfigTreeLoader();

        ~ConfigTreeLoader()=default;
        ConfigTreeLoader(const ConfigTreeLoader&)=default;
        ConfigTreeLoader(ConfigTreeLoader&&)=default;
        ConfigTreeLoader& operator=(const ConfigTreeLoader&)=default;
        ConfigTreeLoader& operator =(ConfigTreeLoader&&)=default;

        void setDefaultFormat(std::string defaultFormat=DefaultFormat)
        {
            m_defaultFormat=std::move(defaultFormat);
        }
        std::string defaultFormat() const
        {
            return m_defaultFormat;
        }

        void setIncludeTag(std::string includeTag=DefaultIncludeTag)
        {
            m_includeTag=std::move(includeTag);
        }
        std::string includeTag() const
        {
            return m_includeTag;
        }

        void setPathSeparator(std::string separator=DefaultPathSeparator)
        {
            m_separator=std::move(separator);
        }
        std::string pathSeparator() const
        {
            return m_separator;
        }

        void setRelFilePathPrefix(std::string prefix=DefaultRelFilePathPrefix)
        {
            m_relFilePathPrefix=std::move(prefix);
        }
        std::string relFilePathPrefix() const
        {
            return m_relFilePathPrefix;
        }

        void setIncludeDirs(std::vector<std::string> dirs)
        {
            m_includeDirs=std::move(dirs);
        }
        const std::vector<std::string>& includeDirs() const
        {
            return m_includeDirs;
        }

        template <typename ...Args>
        void addIncludeDirs(Args&& ...dirs)
        {
            common::ContainerUtils::addElements(m_includeDirs,std::forward<Args>(dirs)...);
        }

        /**
         * @brief Add config tree IO handler that can parse/serialize some formats.
         * @param handler Handler to add.
         *
         * Conflicting formats will be overriden.
         */
        void addHandler(std::shared_ptr<ConfigTreeIo> handler);

        /**
         * @brief Get config tree IO handler for format.
         * @param format Format to look up for. If empty then default.
         * @return Found handler or invalid result.
         */
        std::shared_ptr<ConfigTreeIo> handler(const std::string& format=std::string()) const noexcept;

        Error loadFromFile(
            ConfigTree& target,
            const std::string& filename,
            const ConfigTreePath& root=ConfigTreePath(),
            const std::string& format=std::string()
        ) const;

        Result<ConfigTree> createFromFile(
            const std::string& filename,
            const std::string& format=std::string()
        ) const;

        Error saveToFile(
            const ConfigTree& source,
            lib::string_view filename,
            const ConfigTreePath& root=ConfigTreePath(),
            const std::string& format=std::string()
        ) const;

        void setPrefixSubstitution(
            std::string prefix,
            std::string substitution
        )
        {
            m_prefixSubstitutions.emplace(std::move(prefix),std::move(substitution));
        }

        void unsetPrefixSubstitution(const std::string& prefix)
        {
            m_prefixSubstitutions.erase(prefix);
        }

        const std::map<std::string,std::string>& prefixSubstitutions() const noexcept
        {
            return m_prefixSubstitutions;
        }

    private:

        std::string m_defaultFormat;
        std::string m_includeTag;
        std::string m_separator;
        std::string m_relFilePathPrefix;

        std::map<std::string,std::shared_ptr<ConfigTreeIo>> m_handlers;

        std::vector<std::string> m_includeDirs;

        std::map<std::string,std::string> m_prefixSubstitutions;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREELOADER_H
