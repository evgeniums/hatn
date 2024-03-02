/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file testplugin.сpp
  *
  *      plugin interface.
  *
  */

#include <hatn/common/plugin.h>

using namespace hatn;
using namespace hatn::common;

class TestPlugin : public Plugin
{
    public:

        constexpr static const char* Type="TestPlugin";
        constexpr static const char* Name="hatntestplugin";
        constexpr static const char* Description="Test Plugin";
        constexpr static const char* Vendor="Dracosha";
        constexpr static const char* Revision="1.0.0";

        using Plugin::Plugin;
};

HATN_PLUGIN_EXPORT(TestPlugin)
