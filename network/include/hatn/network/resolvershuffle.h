/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/resolvershuffle.h
  *
  *   Class for shuffling DNS resolver result
  *
  */

/****************************************************************************/

#ifndef HATNRESOLVERSHUFFLE_H
#define HATNRESOLVERSHUFFLE_H

#include <hatn/network/network.h>
#include <hatn/network/asio/ipendpoint.h>

HATN_NETWORK_NAMESPACE_BEGIN

//! Class for shuffling DNS resolver result
class HATN_NETWORK_EXPORT ResolverShuffle final
{
    public:

        enum Mode
        {
            NONE=0,
            RANDOM=0x1,
            APPEND_FALLBACK_PORTS=(0x1<<1),
            RANDOM_APPEND_FALLBACK_PORTS=0x1 | (0x1<<1)
        };

        //! Ctor
        ResolverShuffle():m_mode(Mode::NONE)
        {}

        //! Shuffle DNS result
        /**
         * New items can be added if the shuffler is configured to append fallback ports
         */
        void shuffle(std::vector<asio::IpEndpoint>& endpoints);

        //! Load fallback ports
        inline void loadFallbackPorts(std::vector<uint16_t> ports) noexcept
        {
            m_fallBackPorts=std::move(ports);
        }

        //! Get fallback ports
        inline std::vector<uint16_t> fallbackPorts() const noexcept
        {
            return m_fallBackPorts;
        }

        //! Set shuffle mode
        inline void setMode(Mode mode) noexcept
        {
            m_mode=mode;
        }

        //! Get shuffle mode
        inline Mode mode() const noexcept
        {
            return m_mode;
        }

    private:

        Mode m_mode;
        std::vector<uint16_t> m_fallBackPorts;
};


HATN_NETWORK_NAMESPACE_END

#endif // HATNRESOLVERSHUFFLE_H
