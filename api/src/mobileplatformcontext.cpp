/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file whitemclient/mobileplatformcontext.cpp
  *
  */

#include <hatn/api/client/mobileapp.h>
#include <hatn/api/client/mobileplatformcontext.h>

HATN_API_MOBILECLIENT_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------

MobilePlatformContext::MobilePlatformContext() : m_app(nullptr)
{
}

//-----------------------------------------------------------------------------

MobilePlatformContext::~MobilePlatformContext()
{}

//-----------------------------------------------------------------------------

HATN_NAMESPACE::Error MobilePlatformContext::init(MobileApp* app)
{
    m_app=app;
    return doInit();
}

//-----------------------------------------------------------------------------

HATN_NAMESPACE::Error MobilePlatformContext::doInit()
{
    return HATN_NAMESPACE::OK;
}

//-----------------------------------------------------------------------------

HATN_NAMESPACE::Error MobilePlatformContext::close()
{
    return HATN_NAMESPACE::OK;
}

//-----------------------------------------------------------------------------

HATN_API_MOBILECLIENT_NAMESPACE_END
