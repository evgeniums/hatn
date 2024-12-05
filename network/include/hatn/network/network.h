/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network.h
  *
  *  Hatn Network Library.
  *
  */

/****************************************************************************/

#ifndef HATNNETWORK_H
#define HATNNETWORK_H

#include <hatn/common/logger.h>
#include <hatn/network/config.h>

namespace hatn {
    namespace network {

        // define export symbols fo windows platform
        #ifdef WIN32

        #    ifndef HATN_NETWORK_EXPORT
        #        ifdef BUILD_HATN_NETWORK
        #            define HATN_NETWORK_EXPORT __declspec(dllexport)
        #        else
        #            define HATN_NETWORK_EXPORT __declspec(dllimport)
        #        endif
        #    endif
        #else
        #    define HATN_NETWORK_EXPORT
        #endif
    }
}

DECLARE_LOG_MODULE_EXPORT(network,HATN_NETWORK_EXPORT)

#endif // HATNNETWORK_H
