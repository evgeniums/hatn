/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/*
    
*/
/** @file common/appversion.cpp
 *
 *     Version info for  server.
 *
 */
/****************************************************************************/

#include <assert.h>

#include <hatn/common/translate.h>
#include <hatn/common/appversion.h>

HATN_COMMON_NAMESPACE_BEGIN

ApplicationVersion* Instance=0;

/********************** AppVersion **********************************/

HATN_SINGLETON_INIT(ApplicationVersion)

//---------------------------------------------------------------
std::string ApplicationVersion::releaseName()
{
#ifdef HATN_GENERATE_VERSION
    std::string v(HATN_VERSION_RELEASE_NAME);
    return v;
#else
    return std::string();
#endif
}

//---------------------------------------------------------------
int ApplicationVersion::buildVersion()
{
#ifdef HATN_GENERATE_VERSION
    int v(HATN_VERSION_BUILD);
    return v;
#else
    return 0;
#endif
}

//---------------------------------------------------------------
std::string ApplicationVersion::softwareVersion()
{
#ifdef HATN_GENERATE_VERSION
    std::string v(HATN_VERSION_SOFTWARE);
    return v;
#else
    return std::string();
#endif
}

//---------------------------------------------------------------
std::string ApplicationVersion::toString(const std::string& moduleName)
{
    std::string v;
    v=moduleName+"\n "+_TR("Release")+" "+releaseName()+";\n";
    v+=" "+_TR("Software version")+" "+softwareVersion()+";\n";
    v+=" "+_TR("Build version")+" "+std::to_string(buildVersion())+";\n";
#ifdef _WIN32
    v+=" "+_TR("Windows platform")+";\n";
#else
    v+=" "+_TR("Linux platform")+";\n";
#endif
    return v;
}

//---------------------------------------------------------------
ApplicationVersion* ApplicationVersion::instance(ApplicationVersion *inst)
{
    if (inst)
    {
        delete Instance;
        Instance=inst;
    }
    else
    {
        if (!Instance)
            Instance=new ApplicationVersion();
    }
    return Instance;
}

//---------------------------------------------------------------
void ApplicationVersion::free()
{
    delete Instance;
    Instance=0;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
