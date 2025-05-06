/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*
    
*/
/** @file clientapp/mobileplatfromcontext.h
*/

/****************************************************************************/

#ifndef HATNMOBILEPLATFORMCTX_H
#define HATNMOBILEPLATFORMCTX_H

#include <hatn/common/error.h>

#include <hatn/clientapp/clientappdefs.h>

HATN_CLIENTAPP_MOBILE_NAMESPACE_BEGIN

class MobileApp;

class HATN_CLIENTAPP_EXPORT MobilePlatformContext
{
    public:

        MobilePlatformContext();
        virtual ~MobilePlatformContext();

        MobilePlatformContext(const MobilePlatformContext&)=delete;
        MobilePlatformContext(MobilePlatformContext&&)=default;
        MobilePlatformContext& operator=(const MobilePlatformContext&)=delete;
        MobilePlatformContext& operator=(MobilePlatformContext&&)=default;

        HATN_NAMESPACE::Error init(MobileApp* app);
        virtual HATN_NAMESPACE::Error close();

        MobileApp* app() const noexcept
        {
            return m_app;
        }

    protected:

        virtual HATN_NAMESPACE::Error doInit();

    private:

        MobileApp* m_app;
};

HATN_CLIENTAPP_MOBILE_NAMESPACE_END

#endif // HATNMOBILEPLATFORMCTX_H
