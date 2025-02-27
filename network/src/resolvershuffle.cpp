/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/resolvershuffle.cpp
  *
  *   Class for shuffling DNS resolver result
  *
  */

/****************************************************************************/

#include <hatn/common/random.h>

#include <hatn/network/resolvershuffle.h>

HATN_NETWORK_NAMESPACE_BEGIN
HATN_COMMON_USING

/********************** ResolverShuffle **************************/

//---------------------------------------------------------------
void ResolverShuffle::shuffle(std::vector<asio::IpEndpoint> &endpoints)
{
    if (endpoints.empty())
    {
        return;
    }

    bool shuffleRandom=m_mode&ResolverShuffle::RANDOM;
    bool appendPorts=(m_mode&ResolverShuffle::APPEND_FALLBACK_PORTS)&&!m_fallBackPorts.empty();
    if (shuffleRandom||appendPorts)
    {
        std::vector<asio::IpEndpoint> newEp;
        if (appendPorts)
        {
            newEp.resize(endpoints.size()*(m_fallBackPorts.size()+1));
        }
        else
        {
            newEp.resize(endpoints.size());
        }
        if (shuffleRandom)
        {
            auto offset=common::Random::generate(static_cast<uint32_t>((endpoints.size()))-1);
            for (size_t i=0;i<endpoints.size();i++)
            {
                auto j=static_cast<size_t>((i+offset)%endpoints.size());
                newEp[i]=endpoints[j];
            }
        }
        if (appendPorts)
        {
            for (size_t j=0;j<endpoints.size();j++)
            {
                newEp[j]=endpoints[j];
            }

            for (size_t i=0;i<m_fallBackPorts.size();i++)
            {
                auto offset=(i+1)*endpoints.size();
                auto port=m_fallBackPorts[i];
                for (size_t j=0;j<endpoints.size();j++)
                {
                    auto ep=endpoints[j];
                    ep.setPort(port);
                    newEp[offset+j]=std::move(ep);
                }
            }
        }
        endpoints=newEp;
    }
}

//---------------------------------------------------------------

HATN_NETWORK_NAMESPACE_END
