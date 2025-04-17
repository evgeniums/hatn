/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file serverapp/serverapp.сpp
  *
  */

#include <args.hxx>

#include <boost/asio/signal_set.hpp>

#include <hatn/common/translate.h>
#include <hatn/common/thread.h>

#include <hatn/app/app.h>

#include <hatn/api/server/microservicefactory.h>

#include <hatn/serverapp/serverapp.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

class ServerApp_p
{
    public:

        ServerApp_p(const HATN_APP_NAMESPACE::AppName& appName) : app(appName)
        {}

        void close();

        HATN_APP_NAMESPACE::App app;

        std::map<std::string,std::shared_ptr<HATN_API_NAMESPACE::server::MicroService>> microservices;
};

//--------------------------------------------------------------------------

ServerApp::ServerApp(const HATN_APP_NAMESPACE::AppName& appName)
    : pimpl(std::make_shared<ServerApp_p>(appName))
{}

//--------------------------------------------------------------------------

ServerApp::~ServerApp()
{}

//--------------------------------------------------------------------------

HATN_APP_NAMESPACE::App& ServerApp::app() noexcept
{
    return pimpl->app;
}

//--------------------------------------------------------------------------

const HATN_APP_NAMESPACE::App& ServerApp::app() const noexcept
{
    return pimpl->app;
}

//--------------------------------------------------------------------------

Error ServerApp::initApp(
        int argc,
        char *argv[]
    )
{
    // parse command line arguments

    args::ArgumentParser parser(pimpl->app.appName().displayName);
    args::HelpFlag help(parser, "help", HATN_NAMESPACE::_TR("Display this help message","serverapp"), {'h', "help"});
    args::ValueFlag<std::string> config(parser, "config", HATN_NAMESPACE::_TR("Configuration file","serverapp"), {'c',"config"});
    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help)
    {
        std::cout << parser;
        return OK;
    }
    catch (args::ParseError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return commonError(CommonError::INVALID_COMMAND_LINE_ARGUMENTS);
    }
    catch (args::ValidationError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return commonError(CommonError::INVALID_COMMAND_LINE_ARGUMENTS);
    }

    auto configFilePath=args::get(config);
    if (configFilePath.empty())
    {
        std::cerr << HATN_NAMESPACE::_TR("Error: path to configuration file must be defined in command line arguments with --config","serverapp") << std::endl;
        return commonError(CommonError::INVALID_COMMAND_LINE_ARGUMENTS);
    }

    auto description=fmt::format(HATN_NAMESPACE::_TR("Running \"{}\" with config file \"{}\"","serverapp"),pimpl->app.appName().displayName,configFilePath);
    std::cout << description << std::endl;

    auto runningFailedMessage=HATN_NAMESPACE::_TR("Failed to initialize server application:","serverapp");

    // load config file
    auto ec=pimpl->app.loadConfigFile(configFilePath);
    if (ec)
    {
        std::cerr << runningFailedMessage << " ";
        std::cerr << ec.message() << std::endl;
        return ec;
    }

    // init application
    ec=pimpl->app.init();
    if (ec)
    {
        std::cerr << runningFailedMessage << " ";
        std::cerr << ec.message() << std::endl;
        return ec;
    }

    // done
    return OK;
}

//--------------------------------------------------------------------------

Error ServerApp::initMicroServices(std::shared_ptr<HATN_API_NAMESPACE::server::MicroServiceFactory> factory)
{
    auto r=factory->makeAndRunAll(pimpl->app,pimpl->app.configTree());
    if (r)
    {
        auto initFailedMessage=HATN_NAMESPACE::_TR("Failed to initialize server services:","serverapp");
        std::cerr << initFailedMessage << " ";
        std::cerr << r.error().message() << std::endl;
        return r.takeError();
    }
    pimpl->microservices=r.takeValue();

    return OK;
}

//--------------------------------------------------------------------------

int ServerApp::exec()
{
    auto mainThread=std::make_shared<HATN_COMMON_NAMESPACE::Thread>("main",false);

    boost::asio::signal_set signals(mainThread->asioContextRef(), SIGINT, SIGTERM);
    signals.async_wait(
        [mainThread,pimpl{pimpl}](const boost::system::error_code& ec,
                     int signal_number)
        {
            std::ignore=signal_number;
            if (ec)
            {
                return;
            }

            pimpl->close();
            mainThread->stop();
        }
    );

    HATN_COMMON_NAMESPACE::Thread::setMainThread(mainThread);
    HATN_COMMON_NAMESPACE::Thread::mainThread()->start();

    auto description=fmt::format(HATN_NAMESPACE::_TR("Finished \"{}\"","whitemserver"),pimpl->app.appName().displayName);
    std::cout << description << std::endl;

    return 0;
}

//--------------------------------------------------------------------------

void ServerApp::close()
{
    pimpl->close();
}

//--------------------------------------------------------------------------

void ServerApp::stop()
{
    HATN_COMMON_NAMESPACE::Thread::mainThread()->stop();
}

//--------------------------------------------------------------------------

void ServerApp_p::close()
{
    for (auto&& it: microservices)
    {
        it.second->close();
    }

    app.close();
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END
