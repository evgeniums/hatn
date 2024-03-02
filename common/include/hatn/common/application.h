/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file common/application.h
  *
  *     Base application class.
  *
  */

/****************************************************************************/

#ifndef HATNAPPLICATION_H
#define HATNAPPLICATION_H

#include <string>
#include <memory>

#include <hatn/common/common.h>
#include <hatn/common/logger.h>
#include <hatn/common/parameters.h>
#include <hatn/common/utils.h>


namespace boost {
namespace program_options {
    class options_description;
}
}

DECLARE_LOG_MODULE_EXPORT(application,HATN_COMMON_EXPORT)

HATN_COMMON_NAMESPACE_BEGIN

class AppOptionsBase;
class Options;
class Application_p;
class Environment;

//! Base class for applications
class HATN_COMMON_EXPORT Application
{
    public:

        //! Constructor
        Application(const std::string& execName, const std::string& applicationName);

        //! Destructor
        virtual ~Application();

        Application(const Application&)=delete;
        Application(Application&&)=delete;
        Application& operator=(const Application&)=delete;
        Application& operator=(Application&&)=delete;

#if 0
        virtual ~RuleOf5()=default;
        RuleOf5(const RuleOf5&)=default;
        RuleOf5(RuleOf5&&) =default;
        RuleOf5& operator=(const RuleOf5&)=default;
        RuleOf5& operator=(RuleOf5&&) =default;

        virtual ~RuleOf5Inst();
        RuleOf5Inst(const RuleOf5Inst&);
        RuleOf5Inst(RuleOf5Inst&&) noexcept;
        RuleOf5Inst& operator=(const RuleOf5Inst&);
        RuleOf5Inst& operator=(RuleOf5Inst&&) noexcept;

        RuleOf5::~RuleOf5()=default;
        RuleOf5::RuleOf5(const RuleOf5&)=default;
        RuleOf5::RuleOf5(RuleOf5&&) =default;
        RuleOf5& RuleOf5::operator=(const RuleOf5&)=default;
        RuleOf5& RuleOf5::operator=(RuleOf5&&) =default;

        virtual ~RuleOf5NoCopy()=default;
        RuleOf5NoCopy(const RuleOf5NoCopy&)=delete;
        RuleOf5NoCopy(RuleOf5NoCopy&&) =default;
        RuleOf5NoCopy& operator=(const RuleOf5NoCopy&)=delete;
        RuleOf5NoCopy& operator=(RuleOf5NoCopy&&) =default;

        virtual ~RuleOf5NoCopyNoMove()=default;
        RuleOf5NoCopyNoMove(const RuleOf5NoCopyNoMove&)=delete;
        RuleOf5NoCopyNoMove(RuleOf5NoCopyNoMove&&) =delete;
        RuleOf5NoCopyNoMove& operator=(const RuleOf5NoCopyNoMove&)=delete;
        RuleOf5NoCopyNoMove& operator=(RuleOf5NoCopyNoMove&&) =delete;
#endif

        //! Load options from command line or file on progrum start
        virtual int loadOptionsOnStart(
            int argc,
            char** argv,
            const std::string& defaultConfigFile=std::string(),
            bool allowUnregistered=false
        );

        //! Load options from string data in runtime
        virtual int loadOptionsOnFly(
            const std::string& optionsContent,
            bool allowUnregistered=false,
            bool updateLoggerOptions=false
        );

        //! Load options from in runtime
        virtual int loadOptionsOnFly(
            const std::vector<std::string>& options,
            bool allowUnregistered=false,
            bool updateLoggerOptions=false
        );

        //! Get configuration
        AppOptionsBase* config() const noexcept;

        //! Get environment
        Environment* environment() const noexcept;

        //! Get name
        std::string name() const noexcept;

        //! Get options
        const Options& options() const noexcept;

        //! Execute application
        int exec();

        //! Stop application
        virtual void stop()=0;

    protected:

        //! Run server
        virtual int run()=0;

        //! Set logger handlers
        virtual void setLoggerHandlers();

        //! Create configuration object
        virtual std::shared_ptr<AppOptionsBase> createConfig() const;

        //! Setup environment options to be loaded on the start
        virtual boost::program_options::options_description environmentOptionsOnStart();

        //! Setup environment options to be loaded on the fly
        virtual boost::program_options::options_description environmentOptionsOnFly(bool appendOnStart=false);

        //! Setup application according to changed parameters
        virtual int doSetupOnFly(const ParametersMap& changedParameters, std::string& errorDescription);

        //! Setup application on the start
        virtual int setupOnStart(std::string& errorDescription);

    private:

        template <typename T> int loadOptionsOnFlyImpl(
            const T& options,
            bool allowUnregistered=false,
            bool updateLoggerOptions=false
        );

        boost::program_options::options_description loggerOptions();
        int setupLogger();
        int setupOnFly(const ParametersMap& newParameters, const ParametersMap& oldParameters, std::string& errorDescription, bool forceUpdate=false);

        std::unique_ptr<Application_p> d;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNAPPLICATION_H
