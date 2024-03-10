/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/application.cpp
  *
  *     Base application class.
  *
  */

/****************************************************************************/

#include <iostream>
#include <boost/program_options.hpp>

#include <hatn/common/logger.h>
#include <hatn/common/options.h>
#include <hatn/common/appversion.h>
#include <hatn/common/translate.h>
#include <hatn/common/environment.h>
#include <hatn/common/application.h>

#include <hatn/common/loggermoduleimp.h>
INIT_LOG_MODULE(application,HATN_COMMON_EXPORT)

HATN_COMMON_NAMESPACE_BEGIN

#if __cplusplus < 201703L
static void UnexpectedHandler()
{
    try {
        throw;
    }
    catch(const std::exception &e)
    {
        HATN_FATAL(application,"UNEXPECTED std::exception:" << e.what());
    }
    catch(...)
    {
        HATN_FATAL(application,"UNKNOWN UNEXPECTED EXCEPTION");
    }
    abort();
}
#endif
static void TerminateHandler()
{
    HATN_FATAL(application,"TERMINATE HANDLER");
    abort();
}

/********************** Application **************************/

class Application_p
{
    public:

        std::shared_ptr<AppOptionsBase> config;
        std::string appName;
        std::string execName;

        std::unique_ptr<FileLogHandler> logHandler;
        std::unique_ptr<FileLogHandler> fatalLogHandler;

        Options options;

        explicit Application_p(const std::string& applicationName) : appName(applicationName),
            options([=](){return ApplicationVersion::instance()->toString(appName);})
        {
        }
};

//---------------------------------------------------------------
Application::Application(
        const std::string& execName,
        const std::string& applicationName
    ) : d(std::make_unique<Application_p>(applicationName))
{
    d->execName=execName;
}

//---------------------------------------------------------------
Application::~Application()
{
    HATN_INFO(application,_TR("Good bye"));
    Logger::stop();
    Logger::setOutputHandler(Logger::OutputHandler());
    Logger::setFatalLogHandler(Logger::OutputHandler());
    d->logHandler.reset();
    d->fatalLogHandler.reset();
}

//---------------------------------------------------------------
std::string Application::name() const noexcept
{
    return d->appName;
}

//---------------------------------------------------------------
void Application::setLoggerHandlers()
{
    d->logHandler=std::make_unique<FileLogHandler>(d->config->logFile);
    d->fatalLogHandler=std::make_unique<FileLogHandler>(d->config->fatalLogFile);
    auto* pimpl=d.get();
    Logger::setOutputHandler(
        [pimpl](const FmtAllocatedBufferChar& msg)
        {
            pimpl->logHandler->handler(msg);
        }
    );
    Logger::setFatalLogHandler(
        [pimpl](const FmtAllocatedBufferChar& msg)
        {
            pimpl->fatalLogHandler->handler(msg);
        }
    );
}

//---------------------------------------------------------------
int Application::loadOptionsOnStart(
        int argc,
        char **argv,
        const std::string &defaultConfigFile,
        bool allowUnregistered
    )
{
    // create configurarion object
    d->config=createConfig();

    // fill all options
    auto envOptions=environmentOptionsOnStart();
    envOptions.add(environmentOptionsOnFly(true));
    envOptions.add(loggerOptions());

    // load options
    std::string errorDescription;
    if (int ret=d->options.loadOnStart(envOptions,argc,argv,defaultConfigFile,errorDescription,allowUnregistered)!=Options::CODE_OK)
    {
        std::cerr<<errorDescription<<std::endl;
        return ret;
    }

    // setup logger
    setLoggerHandlers();
    setupLogger();
    Logger::start(d->config->logInSeparateThread);

    // print configuration info
    HATN_INFO(application,name()<<" "<<ApplicationVersion::instance()->releaseName()<<"\n"<<_TR("Configuration:")<<"\n"<<d->options.toString().c_str());

    // setup application depending on the options
    if (int ret=setupOnStart(errorDescription)!=Options::CODE_OK)
    {
        std::cerr<<errorDescription<<std::endl;
        return ret;
    }

    // return ok
    return Options::CODE_OK;
}

//---------------------------------------------------------------
int Application::loadOptionsOnFly(const std::string &optionsContent, bool allowUnregistered, bool updateLoggerOptions)
{
    return loadOptionsOnFlyImpl(optionsContent,allowUnregistered,updateLoggerOptions);
}

//---------------------------------------------------------------
int Application::loadOptionsOnFly(const std::vector<std::string> &options, bool allowUnregistered, bool updateLoggerOptions)
{
    return loadOptionsOnFlyImpl(options,allowUnregistered,updateLoggerOptions);
}

//---------------------------------------------------------------
template <typename T> int Application::loadOptionsOnFlyImpl(const T &optionsContent, bool allowUnregistered, bool updateLoggerOptions)
{
    auto envOptions=environmentOptionsOnFly(false);
    if (updateLoggerOptions)
    {
        envOptions.add(loggerOptions());
    }
    auto oldParameters=d->options.map();

    // load options
    std::string errorDescription;
    if (int ret=d->options.loadOnFly(envOptions,optionsContent,errorDescription,allowUnregistered))
    {
        HATN_ERROR(application,_TR("Failed to load options:")<<"\n"<<errorDescription);
        return ret;
    }
    auto newParameters=d->options.map();

    // setup application depending on the options
    if (int ret=setupOnFly(oldParameters,newParameters,errorDescription)!=Options::CODE_OK)
    {
        HATN_ERROR(application,_TR("Failed to setup application:")<<"\n"<<errorDescription);
        return ret;
    }

    // setup logger
    if (updateLoggerOptions)
    {
        setupLogger();
    }

    // return ok
    return Options::CODE_OK;
}

//---------------------------------------------------------------
boost::program_options::options_description Application::loggerOptions()
{
    boost::program_options::options_description logOptions(_TR("Log options"));
    logOptions.add_options()
        ("log_file",boost::program_options::value(&d->config->logFile)->default_value(d->execName+".log"), _TR("Log file").c_str())
        ("log_file_fatal",boost::program_options::value(&d->config->fatalLogFile)->default_value(d->execName+"-fatal.log"), _TR("Fatal log file").c_str())
        ("log_modules",boost::program_options::value(&d->config->debugModules), _TR("Log configuration for certain modules. Format: module;verbosity;debug level;debug contexts separated by comma;debug tags separated by comma").c_str())
        ("log_verbosity",boost::program_options::value(&d->config->verboseLevel)->default_value("information"), _TR("Verbosity level of log messages: debug|information|warning|error|fatal").c_str())
        ("log_debug_level",boost::program_options::value(&d->config->debugLevel)->default_value(d->config->debugLevel), _TR("Debug level of debug messages").c_str())
        ("log_thread",boost::program_options::value(&d->config->logInSeparateThread)->default_value(d->config->logInSeparateThread), _TR("Log in separate thread").c_str())
        ("log_trace_fatal",boost::program_options::value(&d->config->traceFatalLogs)->default_value(d->config->traceFatalLogs), _TR("Trace fatal logs").c_str())
        ("log_file_append",boost::program_options::value(&d->config->logFileAppend)->default_value(d->config->logFileAppend), _TR("Append logs to log files after restart").c_str())
        ("log_to_stderr",boost::program_options::value(&d->config->logToStd)->default_value(d->config->logToStd), _TR("Also log to console stderr stream").c_str())
        ;
    return logOptions;
}

//---------------------------------------------------------------
int Application::setupLogger()
{
    Logger::setDefaultVerbosity(Logger::stringToVerbosity(d->config->verboseLevel.c_str()));
    auto loggerModulesStatus=Logger::configureModules(d->config->debugModules);
    if (!loggerModulesStatus.empty())
    {
        std::cerr<<loggerModulesStatus<<std::endl;
        return Options::CODE_INVALID_LOGGER_CONFIGURATION;
    }
    Logger::setFatalTracing(d->config->traceFatalLogs);
    Logger::setAppendLogFile(d->config->logFileAppend);
    Logger::setLogToStd(d->config->logToStd);

    return Options::CODE_OK;
}

//---------------------------------------------------------------
boost::program_options::options_description Application::environmentOptionsOnStart()
{
    return boost::program_options::options_description();
}

//---------------------------------------------------------------
boost::program_options::options_description Application::environmentOptionsOnFly(bool appendOnStart)
{
    std::ignore=appendOnStart;
    return boost::program_options::options_description();
}

//---------------------------------------------------------------
int Application::setupOnFly(
        const ParametersMap &newParameters,
        const ParametersMap &oldParameters,
        std::string &errorDescription,
        bool forceUpdate
    )
{
    if (forceUpdate)
    {
        return doSetupOnFly(newParameters,errorDescription);
    }

    ParametersMap changed;
    std::set<std::string> skipMultiKeys;
    for (auto&& it:newParameters)
    {
        auto it2=skipMultiKeys.find(it.first);
        if (it2!=skipMultiKeys.end())
        {
            continue;
        }

        auto newCount=newParameters.count(it.first);
        auto oldCount=oldParameters.count(it.first);
        if (newCount>1 || oldCount>1)
        {
            // process multiple keys
            skipMultiKeys.insert(it.first);
            if (newCount!=oldCount)
            {
                auto range=newParameters.equal_range(it.first);
                for (auto it3=range.first;it3!=range.second;++it3)
                {
                    changed.insert(std::make_pair(it3->first,it3->second));
                }
            }
            else
            {
                bool valueChanged=false;
                auto range1=newParameters.equal_range(it.first);
                for (auto it3=range1.first;it3!=range1.second;++it3)
                {
                    bool hasValue=false;
                    auto range2=oldParameters.equal_range(it.first);
                    for (auto it4=range2.first;it4!=range1.second;++it4)
                    {
                        if (Utils::compareBoostAny(it4->second,it3->second))
                        {
                            hasValue=true;
                            break;
                        }
                    }
                    if (!hasValue)
                    {
                        valueChanged=true;
                        break;
                    }
                }
                if (valueChanged)
                {
                    for (auto it3=range1.first;it3!=range1.second;++it3)
                    {
                        changed.insert(std::make_pair(it.first,it3->second));
                    }
                }
            }
        }
        else
        {
            // process single keys
            auto it1=oldParameters.find(it.first);
            if (it1!=oldParameters.end())
            {
                if (!Utils::compareBoostAny(it.second,it1->second))
                {
                    changed[it.first]=it1->second;
                }
            }
            else
            {
                changed[it.first]=it.second;
            }
        }
    }
    if (!changed.empty())
    {
        return doSetupOnFly(changed,errorDescription);
    }
    return Options::CODE_OK;
}

//---------------------------------------------------------------
int Application::doSetupOnFly(const ParametersMap &changedParameters, std::string &errorDescription)
{
    std::ignore=changedParameters;
    std::ignore=errorDescription;
    return Options::CODE_OK;
}

//---------------------------------------------------------------
int Application::setupOnStart(std::string &errorDescription)
{
    std::ignore=errorDescription;
    return Options::CODE_OK;
}

//---------------------------------------------------------------
const Options& Application::options() const noexcept
{
    return d->options;
}

//---------------------------------------------------------------
AppOptionsBase* Application::config() const noexcept
{
    return d->config.get();
}

//---------------------------------------------------------------
Environment* Application::environment() const noexcept
{
    return d->config->environment.get();
}

//---------------------------------------------------------------
std::shared_ptr<AppOptionsBase> Application::createConfig() const
{
    auto config=std::make_shared<AppOptionsBase>();
    config->environment=std::make_shared<Environment>();
    return config;
}

//---------------------------------------------------------------
int Application::exec()
{
#if __cplusplus < 201703L
    std::set_unexpected(&UnexpectedHandler);
#endif
    std::set_terminate(&TerminateHandler);
    int ret=0;
    try
    {
        ret=run();
    }
    catch(std::exception &e)
    {
        HATN_FATAL(application,"Uncaught exception: " << e.what());
        return EXIT_FAILURE;
    }
    return ret;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
