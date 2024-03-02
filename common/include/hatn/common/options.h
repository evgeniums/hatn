/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/options.h
  *
  *      Options.
  *
  */

/****************************************************************************/

#ifndef HATNOPTIONS_H
#define HATNOPTIONS_H

#include <set>
#include <string>
#include <memory>
#include <functional>

#include <hatn/common/common.h>
#include <hatn/common/environment.h>

#include <hatn/common/parameters.h>

namespace boost {
namespace program_options {
    class options_description;
}
}

HATN_COMMON_NAMESPACE_BEGIN

class Options_p;

using InfoCb=std::function<std::string ()>;

//! Base aplication options
class HATN_COMMON_EXPORT AppOptionsBase
{
    public:

        std::string logFile;
        std::string fatalLogFile;
        bool traceFatalLogs;

        std::string verboseLevel;
        int debugLevel;
        bool logInSeparateThread;
        std::vector<std::string> debugModules;

        std::shared_ptr<Environment> environment;

        bool logFileAppend;
        bool logToStd;

        //! Constructor
        AppOptionsBase();

        virtual ~AppOptionsBase()=default;
        AppOptionsBase(const AppOptionsBase&)=default;
        AppOptionsBase(AppOptionsBase&&)=default;
        AppOptionsBase& operator=(const AppOptionsBase&)=default;
        AppOptionsBase& operator=(AppOptionsBase&&)=default;
};

//!  options
class HATN_COMMON_EXPORT Options : public Parameters
{
    public:

        //! Error codes
        enum ErrorCodes
        {
            CODE_OK,
            CODE_PRINT_HELP,
            CODE_PRINT_VERSION,
            CODE_INVALID_CONFIGURATION,
            CODE_INVALID_LOGGER_CONFIGURATION
        };

        //! Constructor
        Options(
            InfoCb infoCallback=InfoCb(),
            std::string confFileKey="config"
        );

        virtual ~Options();
        Options(const Options&)=delete;
        Options(Options&&) noexcept;
        Options& operator=(const Options&)=delete;
        Options& operator=(Options&&) noexcept;

        //! Load options from boost program options on the start
        int loadOnStart(
            const boost::program_options::options_description& envOptions,
            int argc,
            char **argv,
            const std::string& defaultConfigFile,
            std::string& errorDescription,
            bool allowUnregistered
        );

        //! Split string in INI file format to map
        static std::map<std::string, std::string> splitIniStringToOptionsMap(const std::string& content);

        //! Load options from boost program options and string content
        int loadOnFly(
            const boost::program_options::options_description& envOptions,
            const std::string& optionsContent,
            std::string& errorDescription,
            bool allowUnregistered
        );

        //! Load options from parsed list
        int loadOnFly(
            const boost::program_options::options_description& envOptions,
            const std::vector<std::string>& options,
            std::string& errorDescription,
            bool allowUnregistered
        );

        //! Print configuration to string
        std::string toString(const std::set<std::string>& excludeKeys=std::set<std::string>()) const;

    private:

        std::unique_ptr<Options_p> d;
};

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNOPTIONS_H
