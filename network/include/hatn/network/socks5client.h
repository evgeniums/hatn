/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/socks5client.h
  *
  *   Class to parse and construct messages of SOCKS5 protocol
  *
  */

/****************************************************************************/

#ifndef HATNNETWORKSOCKS5_H
#define HATNNETWORKSOCKS5_H

#include <functional>

#include <hatn/common/error.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/network/network.h>
#include <hatn/network/asio/ipendpoint.h>

HATN_NETWORK_NAMESPACE_BEGIN

    /**
     * @brief Parse and construct messages of SOCKS5 protocol.
     *
     * This class only processes requests and responses depending on the current state and data of the last response.
     * It does not make any system calls, neither it handles any sockets.
     * Just provide responses received from SOCKS5 server to its input and get back prepared requests
     * with actual status and the next step to do.
     * Network operations must be performed somewhere else.
     *
     */
    class HATN_NETWORK_EXPORT Socks5Client
    {
        public:

            //! Connection parameters
            struct HATN_NETWORK_EXPORT Target
            {
                asio::IpEndpoint endpoint;
                std::string domain;

                inline void clear() noexcept
                {
                    endpoint.clear();
                    domain.clear();
                }

                inline bool isValid() noexcept
                {
                    return endpoint.port()!=0
                            &&
                            (!endpoint.address().is_unspecified()
                             ||
                             (!domain.empty() && domain.size()<255)
                             )
                            ;
                }
            };

            //! Status of last request/response
            enum class StepStatus : int
            {
                Done=0,
                SendAndReceive=1,
                Receive=2,
                Fail=3
            };

            //! Constructor
            Socks5Client(
                common::pmr::AllocatorFactory* allocatorFactory=common::pmr::AllocatorFactory::getDefault()
            );

            //! Get buffer where to put responses
            inline common::ByteArray&  responseBuffer() noexcept
            {
                return m_response;
            }

            //! Get buffer with request to be sent to proxy server
            inline const common::ByteArray&  requestBuffer() const noexcept
            {
                return m_request;
            }

            //! Process next request
            /**
             * Fills request buffer if next request is valid.
             * @return Status saying how the next request must be handled.
             */
            StepStatus nextRequest();

            //! Get last error
            inline common::Error lastError() const noexcept
            {
                return m_error;
            }

            //! Get result
            inline Target result() const noexcept
            {
                return m_result;
            }

            //! Set destination
            /**
             * Calling this method resets the state.
             */
            inline void setDestination(
                asio::IpEndpoint destination,
                std::string domain=std::string()
            ) noexcept
            {
                Target dest;
                dest.endpoint=std::move(destination);
                dest.domain=std::move(domain);
                setDestination(dest);
            }

            //! Set destination
            /**
             * Calling this method resets the state.
             */
            inline void setDestination(
                std::string domain,
                uint16_t port
            ) noexcept
            {
                Target dest;
                dest.endpoint.setPort(port);
                dest.domain=std::move(domain);
                setDestination(dest);
            }

            //! Set destination
            /**
             * Calling this method resets the state.
             */
            inline void setDestination(
                Target destination
            ) noexcept
            {
                m_destination=std::move(destination);
                reset();
            }

            //! Set UDP mode
            inline void setUdp(bool enable) noexcept
            {
                m_udp=enable;
            }

            //! Reset state
            void reset() noexcept;

            //! Reset buffers
            /**
             * @attention This method invalidates buffers, so never call it while any async operation with the buffers is in progress.
             * Note, that the buffers will be auto reset on success when initialization of connection to proxy server is finished.
             */
            inline void resetBuffers() noexcept
            {
                m_request.reset();
                m_response.reset();
            }

            /**
             * @brief setCredentials
             * @param login
             * @param password
             */
            void setCredentials(
                    std::string login,
                    std::string password
                ) noexcept
            {
                m_login=std::move(login);
                m_password=std::move(password);
            }

            /**
             * @brief Wrap UDP packet before sending to proxy
             * @param destination Destination of the UDP packet
             * @param packet UDP packet
             */
            static void wrapUdpPacket(const Target& destination, common::ByteArray& packet);

            /**
             * @brief Unwrap UDP packet received from proxy
             * @param source Source of the UDP packet
             * @param packet UDP packet
             */
            static bool unwrapUdpPacket(Target &source, common::ByteArray& packet);

        private:

            void sendConnectRequest() noexcept;

            enum class State:int
            {
                Idle=0,
                Negotiation=1,
                Auth=2,
                Connect=3,
                ReceiveAddress=4,
                Done=5
            };

            State m_state;
            common::Error m_error;
            Target m_destination;

            common::ByteArray m_request;
            common::ByteArray m_response;

            std::string m_login;
            std::string m_password;

            bool m_udp;

            uint8_t m_addressType;
            uint8_t m_addressLength;

            Target m_result;
    };


HATN_NETWORK_NAMESPACE_END

#endif // HATNNETWORKSOCKS5_H
