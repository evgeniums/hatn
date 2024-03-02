/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/options.сpp
  *
  *      Options.
  *
  */

#include <istream>
#include <fstream>
#include <iostream>

#include <map>
#include <boost/thread/locks.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>

#include <hatn/common/types.h>
#include <hatn/common/utils.h>
#include <hatn/common/translate.h>
#include <hatn/common/options.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace {

/* Part of parseOptionsContent() is copy-pasted from Boost's common_config_file_iterator::get */

std::string trim_ws(const std::string& s)
{
    std::string::size_type n, n2;
    n = s.find_first_not_of(" \t\r\n");
    if (n == std::string::npos)
        return std::string();
    else {
        n2 = s.find_last_not_of(" \t\r\n");
        return s.substr(n, n2-n+1);
    }
}
std::map<std::string, std::string> optionsStringToMap(const std::string& str)
{
    std::istringstream ss(str);

    std::string s, m_prefix;
    std::map<std::string, std::string> optionsMap;
    std::string::size_type n;

    while(std::getline(ss, s))
    {
        // strip '#' comments and whitespace
        if ((n = s.find('#')) != std::string::npos)
            s = s.substr(0, n);
        s = trim_ws(s);

        if (!s.empty())
        {
            // Handle section name
            if (*s.begin() == '[' && *s.rbegin() == ']')
            {
                m_prefix = s.substr(1, s.size()-2);
                if (*m_prefix.rbegin() != '.')
                    m_prefix += '.';
            }
            else if ((n = s.find('=')) != std::string::npos)
            {
                std::string name = m_prefix + trim_ws(s.substr(0, n));
                std::string value = trim_ws(s.substr(n+1));
                optionsMap.insert(std::make_pair(name,value));
            }
        }
    }
    return optionsMap;
}
std::vector<std::string> optionsMapToVector(const std::map<std::string, std::string>& optionsMap)
{
    std::vector<std::string> result;
    for (auto&& it: optionsMap)
    {
        result.push_back("--" + it.first);
        result.push_back(it.second);
    }
    return result;
}

}

/********************** AppOptionsBase **************************/

//---------------------------------------------------------------
AppOptionsBase::AppOptionsBase(
    ):traceFatalLogs(true),debugLevel(0),logInSeparateThread(true)
#ifdef HATN_LOG_FILE_TRUNCATE
    ,logFileAppend(false)
#else
    ,logFileAppend(true)
#endif
#ifdef HATN_LOG_TO_STD
  ,logToStd(true)
#else
  ,logToStd(false)
#endif
{
}

/********************** Options **************************/

class Options_p
{
    public:

        InfoCb infoCallback;
        std::string confFileKey;
        boost::program_options::variables_map vm;
};

//---------------------------------------------------------------
Options::~Options()
{}

//---------------------------------------------------------------
Options::Options(Options&& other) noexcept : d(std::move(other.d))
{}

//---------------------------------------------------------------
Options& Options::operator=(Options&& other) noexcept
{
    if (&other!=this)
    {
        d=std::move(other.d);
    }
    return *this;
}

//---------------------------------------------------------------
Options::Options(
        InfoCb infoCallback,
        std::string confFileKey
    ) : d(std::make_unique<Options_p>())
{
    d->infoCallback=std::move(infoCallback);
    d->confFileKey=std::move(confFileKey);
}

//---------------------------------------------------------------
int Options::loadOnFly(
        const boost::program_options::options_description &envOptions,
        const std::string &optionsContent,
        std::string &errorDescription,
        bool allowUnregistered
    )
{
    std::vector<std::string> options=optionsMapToVector(optionsStringToMap(optionsContent));
    return loadOnFly(envOptions,options,errorDescription,allowUnregistered);
}

//---------------------------------------------------------------
int Options::loadOnFly(
        const boost::program_options::options_description &envOptions,
        const std::vector<std::string> &options,
        std::string &errorDescription,
        bool allowUnregistered
    )
{
    // parse options
    boost::program_options::variables_map vm;
    try
    {
        if (allowUnregistered)
        {
            boost::program_options::store(boost::program_options::command_line_parser(options).options(envOptions).allow_unregistered().run(),vm);
        }
        else
        {
            boost::program_options::store(boost::program_options::command_line_parser(options).options(envOptions).run(),vm);
        }
        boost::program_options::notify(vm);
    }
    catch(std::exception &e)
    {
        std::stringstream ss;
        ss << e.what() << std::endl;
        ss << envOptions << std::endl;
        errorDescription=ss.str();
        return CODE_INVALID_CONFIGURATION;
    }

    // copy options to map
    for (const auto& it1 : vm)
    {
        setValue(it1.first,it1.second);
        d->vm.erase(it1.first);
    }
    d->vm.insert(vm.begin(),vm.end());

    // return result
    return CODE_OK;
}

//---------------------------------------------------------------
int Options::loadOnStart(
        const boost::program_options::options_description &envOptions,
        int argc,
        char **argv,
        const std::string &defaultConfigFile,
        std::string& errorDescription,
        bool allowUnregistered
    )
{
    std::stringstream ss;

    // fill command line options
    std::string configFile=defaultConfigFile;
    boost::program_options::options_description cmdLineOptions(_TR("Generic options"));
    cmdLineOptions.add_options()
            ("help,h", _TR("Print this help message").c_str())
            ("version,v", _TR("Show version").c_str())
            ("config,c", boost::program_options::value(&configFile)->default_value(configFile),_TR("Configuration file name").c_str())
    ;

    // check generic command line options
    boost::program_options::variables_map vm1;
    try
    {
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(cmdLineOptions).allow_unregistered().run(),vm1);
        if (vm1.count(_TR("help")))
        {
            if (d->infoCallback)
            {
                ss<<d->infoCallback()<<std::endl;
            }
            ss << cmdLineOptions << envOptions << std::endl;
            errorDescription=ss.str();
            return CODE_PRINT_HELP;
        }
        if (vm1.count(_TR("version")))
        {
            if (d->infoCallback)
            {
                ss<<d->infoCallback()<<std::endl;
            }
            errorDescription=ss.str();
            return CODE_PRINT_VERSION;
        }
        boost::program_options::notify(vm1);
    }
    catch(std::exception &e)
    {
        ss << e.what() << std::endl;
        ss << cmdLineOptions << std::endl;
        errorDescription=ss.str();
        return CODE_INVALID_CONFIGURATION;
    }

    // check config file
    if (vm1.count(d->confFileKey))
    {
        auto it=vm1.find(d->confFileKey);
        if (it!=vm1.end())
        {
            std::string cf=it->second.as<std::string>();
            if (!cf.empty())
            {
                configFile=cf;
            }
        }
    }

    // parse all options
    auto allOptions=envOptions;
    allOptions.add(cmdLineOptions);
    boost::program_options::variables_map vm;
    try
    {
        if (allowUnregistered)
        {
            boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(allOptions).allow_unregistered().run(),vm);
        }
        else
        {
            boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(allOptions).run(),vm);
        }
        bool checkFile=true;
        if (configFile==defaultConfigFile)
        {
            if (configFile.empty())
            {
                checkFile=false;
            }
        }
        if (checkFile)
        {
            if (!boost::filesystem::exists(configFile))
            {
                errorDescription=std::string(_TR("Сonfiguration file"))+std::string(" ")+configFile+std::string(" ")+_TR("not found");
                return CODE_INVALID_CONFIGURATION;
            }
            std::ifstream file(configFile.c_str());
            if(file)
            {
                boost::program_options::store(boost::program_options::parse_config_file(file,allOptions,allowUnregistered),vm);
            }
            else
            {
                errorDescription=std::string(_TR("Failed to read configuration file"))+std::string(" ")+configFile;
                return CODE_INVALID_CONFIGURATION;
            }
        }
        boost::program_options::notify(vm);
    }
    catch(std::exception &e)
    {
        ss << e.what() << std::endl;
        ss << allOptions << std::endl;
        errorDescription=ss.str();
        return CODE_INVALID_CONFIGURATION;
    }

    // copy options to map
    for (const auto& it1 : vm)
    {
        setValue(it1.first,it1.second);
    }
    d->vm=vm;

    // return result
    return CODE_OK;
}

//---------------------------------------------------------------
std::string Options::toString(
        const std::set<std::string> &excludeKeys
    ) const
{
    std::stringstream ss;
    for(auto it = d->vm.begin(); it != d->vm.end(); ++it )
    {
        if (excludeKeys.find(it->first)==excludeKeys.end())
        {
            const boost::program_options::variable_value &value = it->second;
            if ( !value.empty() )
            {
                const std::type_info &type = value.value().type();
                std::string val;
                if( type == typeid(std::string) )
                {
                    if( type == typeid(int) )
                    {
                        int i = value.as<int>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(unsigned int) )
                    {
                        unsigned int i = value.as<unsigned int>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(uint32_t) )
                    {
                        uint32_t i = value.as<uint32_t>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(int32_t) )
                    {
                        int32_t i = value.as<int32_t>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(char))
                    {
                        int i = static_cast<int>(value.as<char>());
                        val = std::to_string(i);
                    }
                    else if( type == typeid(unsigned char))
                    {
                        unsigned int i = static_cast<unsigned int>(value.as<unsigned char>());
                        val = std::to_string(i);
                    }
                    else if( type == typeid(uint16_t))
                    {
                        unsigned int i = value.as<uint16_t>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(int16_t))
                    {
                        int16_t i = value.as<int16_t>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(uint16_t))
                    {
                        int i = value.as<uint16_t>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(int64_t))
                    {
                        int64_t i = value.as<int64_t>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(unsigned int))
                    {
                        unsigned int i = value.as<unsigned int>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(bool))
                    {
                        bool i = value.as<bool>();
                        val = i?"true":"false";
                    }
                    else if( type == typeid(double))
                    {
                        double i = value.as<double>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(float))
                    {
                        float i = value.as<float>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(long long))
                    {
                        long long i = value.as<long long>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(unsigned long long))
                    {
                        unsigned long long i = value.as<unsigned long long>();
                        val = std::to_string(i);
                    }
                    else if( type == typeid(std::vector<std::string>))
                    {
                        const auto& i = value.as<std::vector<std::string> >();
                        val = boost::algorithm::join(i, ", ");
                    }
                    else
                    {
                        val = ": ???";
                    }
                    ss<<" "<<it->first<<": "<<val<<std::endl;
                }
            }
        }
    }
    ss<<std::endl;
    return ss.str();
}

//---------------------------------------------------------------
std::map<std::string, std::string> Options::splitIniStringToOptionsMap(const std::string &content)
{
    return optionsStringToMap(content);
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
