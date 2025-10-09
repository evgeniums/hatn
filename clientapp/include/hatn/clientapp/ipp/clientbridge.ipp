/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/ipp/clientbridge.ipp
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTBRIDGE_IPP
#define HATNCLIENTBRIDGE_IPP

#include <hatn/clientapp/clientbridge.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ServiceName, typename ControllerT>
ServiceBaseT<ServiceName,ControllerT>::~ServiceBaseT()
{
}

//--------------------------------------------------------------------------

template <typename ServiceName, typename ControllerT, typename ClientAppT>
ServiceT<ServiceName,ControllerT,ClientAppT>::ServiceT(ClientApp* app) : ServiceT(std::make_shared<Controller>(app))
{}

//--------------------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTBRIDGE_IPP
