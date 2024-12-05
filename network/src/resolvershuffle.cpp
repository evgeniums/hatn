/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/resolvershuffle.cpp
  *
  *   Class for shuffling DNS resolver result
  *
  */

/****************************************************************************/

#include <hatn/network/resolvershuffle.h>

namespace hatn {

using namespace common;

namespace network {

/********************** ResolverShuffle **************************/

//---------------------------------------------------------------
void ResolverShuffle::shuffle(std::vector<asio::IpEndpoint> &endpoints)
{
    bool shuffleRandom=m_mode&ResolverShuffle::RANDOM;
    bool appendPorts=(m_mode&ResolverShuffle::APPEND_FALLBACK_PORTS)&&!m_fallBackPorts.empty();
    if (shuffleRandom||appendPorts)
    {
        std::vector<asio::IpEndpoint> newEp(endpoints.size()*(m_fallBackPorts.size()+1));
        if (shuffleRandom)
        {
            auto offset=rand()%endpoints.size();
            for (size_t i=0;i<endpoints.size();i++)
            {
                auto j=static_cast<size_t>((i+offset)%endpoints.size());
                newEp[i]=endpoints[j];
            }
        }
        if (appendPorts)
        {
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
} // namespace network
} // namespace hatn
