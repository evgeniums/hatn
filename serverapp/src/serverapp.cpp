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

// #include <args.hxx>

#include <boost/asio/signal_set.hpp>

#include <hatn/common/translate.h>
#include <hatn/common/thread.h>

#include <hatn/app/baseapp.h>

#include <hatn/serverapp/serverapp.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

class ServerApp_p
{
    public:

        ServerApp_p(const HATN_APP_NAMESPACE::AppName& appName) : app(appName)
        {}

        void close();

        HATN_APP_NAMESPACE::BaseApp app;

        std::shared_ptr<HATN_API_NAMESPACE::server::MicroServiceFactory> factory;
        std::shared_ptr<std::map<std::string,std::shared_ptr<HATN_API_NAMESPACE::server::MicroService>>> microservices;
};

//--------------------------------------------------------------------------

ServerApp::ServerApp(const HATN_APP_NAMESPACE::AppName& appName)
    : pimpl(std::make_shared<ServerApp_p>(appName))
{}

//--------------------------------------------------------------------------

ServerApp::~ServerApp()
{}

//--------------------------------------------------------------------------

HATN_APP_NAMESPACE::BaseApp& ServerApp::app()
{
    return pimpl->app;
}
#if 0
//--------------------------------------------------------------------------

int ServerApp::init(
        int argc,
        char *argv[]
    )
{
    // parse command line arguments

    args::ArgumentParser parser(pimpl->app.appName().displayName);
    args::HelpFlag help(parser, "help", HATN_NAMESPACE::_TR("Display this help message","whitemserver"), {'h', "help"});
    args::ValueFlag<std::string> config(parser, "config", HATN_NAMESPACE::_TR("Configuration file","whitemserver"), {'c',"config"});
    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help)
    {
        std::cout << parser;
        return 0;
    }
    catch (args::ParseError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    catch (args::ValidationError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 2;
    }

    auto configFilePath=args::get(config);
    if (configFilePath.empty())
    {
        std::cerr << HATN_NAMESPACE::_TR("Error: path to configuration file must be defined in command line arguments with --config","whitemserver") << std::endl;
        return 3;
    }

    auto description=fmt::format(HATN_NAMESPACE::_TR("Running \"{}\" with config file \"{}\"","whitemserver"),pimpl->app.appName().displayName,configFilePath);
    std::cout << description << std::endl;

    auto runningFailedMessage=HATN_NAMESPACE::_TR("Failed to run server:","whitemserver");

    // load config file
    auto ec=pimpl->app.loadConfigFile(configFilePath);
    if (ec)
    {
        std::cerr << runningFailedMessage << " ";
        std::cerr << ec.message() << std::endl;
        return ec.value();
    }

    // init application
    ec=pimpl->app.init();
    if (ec)
    {
        std::cerr << runningFailedMessage << " ";
        std::cerr << ec.message() << std::endl;
        return ec.value();
    }

    //! @todo init API server

    // done
    return 0;
}
#endif
//--------------------------------------------------------------------------

void ServerApp::exec()
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
    app.close();
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END
