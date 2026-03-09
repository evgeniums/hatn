/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/protomessagemap.сpp
  *
  */

#include <hatn/clientserver/protomessagemap.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

/*****************************ProtoMessageMap********************************/

HATN_SINGLETON_INIT(ProtoMessageMap)

//--------------------------------------------------------------------------

ProtoMessageMap& ProtoMessageMap::instance()
{
    static ProtoMessageMap inst;
    return inst;
}

//--------------------------------------------------------------------------

void ProtoMessageMap::free()
{
    instance().m_cppByProto.clear();
    instance().m_protoByCpp.clear();
}

//--------------------------------------------------------------------------

void ProtoMessageMap::registerMessage(std::string cpp, std::string proto)
{
    m_protoByCpp[cpp]=proto;
    m_cppByProto[proto]=cpp;
}

//--------------------------------------------------------------------------

std::string ProtoMessageMap::findCppByProto(const std::string& name) const
{
    auto it=m_cppByProto.find(name);
    if (it!=m_cppByProto.end())
    {
        return it->second;
    }
    return name;
}

//--------------------------------------------------------------------------

std::string ProtoMessageMap::findProtoByCpp(const std::string& name) const
{
    auto it=m_protoByCpp.find(name);
    if (it!=m_protoByCpp.end())
    {
        return it->second;
    }
    return name;
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END
