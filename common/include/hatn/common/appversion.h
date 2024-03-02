/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/appversion.h
 *
 *     Version info of the application.
 *
 */
/****************************************************************************/

#ifndef HATNAPPVERSION_H
#define HATNAPPVERSION_H

#include <string>

#ifdef HATN_GENERATE_VERSION
#include "df_version.h"
#endif

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/singleton.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Application's version info
class HATN_COMMON_EXPORT ApplicationVersion : public Singleton
{
    public:

        HATN_SINGLETON_DECLARE()

        ApplicationVersion()=default;

        virtual std::string releaseName();
        virtual std::string softwareVersion();
        virtual int buildVersion();

        virtual std::string toString(const std::string& appName);

    public:

        static ApplicationVersion* instance(ApplicationVersion* inst=0);
        static void free();
};

template <typename T> class AppVersion : public ApplicationVersion
{
    virtual std::string releaseName()
    {
    #ifdef HATN_GENERATE_VERSION
        std::string v(HATN_VERSION_RELEASE_NAME);
        return v;
    #else
        return AppVersion::releaseName();
    #endif
    }

    virtual std::string softwareVersion()
    {
    #ifdef HATN_GENERATE_VERSION
        std::string v(HATN_VERSION_SOFTWARE);
        return v;
    #else
        return AppVersion::softwareVersion();
    #endif
    }

    virtual int buildVersion()
    {
    #ifdef HATN_GENERATE_VERSION
        int v(HATN_VERSION_BUILD);
        return v;
    #else
        return AppVersion::buildVersion();
    #endif
    }
};


HATN_COMMON_NAMESPACE_END

#endif // HATNAPPVERSION_H
