/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/asio/tcpserver.h
  *
  *   TCP server built on boost asio
  *
  */

/****************************************************************************/

#ifndef HATNASIOTCPSERVER_H
#define HATNASIOTCPSERVER_H

#include <hatn/common/error.h>
#include <hatn/common/environment.h>
#include <hatn/common/interface.h>
#include <hatn/common/objectid.h>
#include <hatn/common/objectguard.h>

#include <hatn/network/network.h>
#include <hatn/network/asio/ipendpoint.h>
#include <hatn/network/asio/socket.h>

DECLARE_LOG_MODULE_EXPORT(asiotcpserver,HATN_NETWORK_EXPORT)

namespace hatn {
namespace network {
namespace asio {

class TcpServerConfig;
class TcpServer_p;

//! TCP server built on boost asio
class HATN_NETWORK_EXPORT TcpServer final
        : public common::WithGuard,
          public common::WithIDThread
{
    public:

        using Callback=std::function<void (const common::Error&)>;

        //! Constructor
        TcpServer(common::STR_ID_TYPE id=common::STR_ID_TYPE());

        //! Constructor
        TcpServer(
            common::Thread* thread,
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        );

        //! Constructor
        TcpServer(
            TcpServerConfig* config,
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        );

        //! Constructor
        TcpServer(
            TcpServerConfig* config,
            common::Thread* thread,
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        );

        //! Destructor is intentionally not virtual as is is a final class
        ~TcpServer();

        TcpServer(const TcpServer&)=delete;
        TcpServer(TcpServer&&) =default;
        TcpServer& operator=(const TcpServer&)=delete;
        TcpServer& operator=(TcpServer&&) =default;

        //! Listen for incoming connections
        common::Error listen(
            const TcpEndpoint& endpoint
        );

        //! Accept connection
        void accept(
            TcpSocket& socket,
            Callback callback
        );

        //! Close
        common::Error close();

    private:

        std::unique_ptr<TcpServer_p> d;
};

} // namespace asio
} // namespace network
} // namespace hatn

#endif // HATNASIOTCPSERVER_H
