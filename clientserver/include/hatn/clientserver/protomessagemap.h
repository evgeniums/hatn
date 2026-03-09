/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file clientserver/protomessagemap.h
  */

/****************************************************************************/

#ifndef HATNPROTOMESSAGEMAP_H
#define HATNPROTOMESSAGEMAP_H

#include <hatn/common/singleton.h>
#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

class HATN_CLIENT_SERVER_EXPORT ProtoMessageMap : public common::Singleton
{
    public:

        HATN_SINGLETON_DECLARE()

        static ProtoMessageMap& instance();
        static void free();

        void registerMessage(std::string cpp, std::string proto);

        std::string findCppByProto(const std::string& name) const;
        std::string findProtoByCpp(const std::string& name) const;

    private:

        ProtoMessageMap()=default;

        std::map<std::string,std::string,std::less<>> m_cppByProto;
        std::map<std::string,std::string,std::less<>> m_protoByCpp;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNPROTOMESSAGEMAP_H
