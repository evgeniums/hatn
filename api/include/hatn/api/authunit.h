/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/authunit.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIAUTHUNIT_H
#define HATNAPIAUTHUNIT_H

#include <hatn/dataunit/syntax.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>
#include <hatn/api/protocol.h>

HATN_API_NAMESPACE_BEGIN

HDU_UNIT(auth_protocol,
    HDU_FIELD(protocol,HDU_TYPE_FIXED_STRING(protocol::AuthProtocolNameLengthMax),1)
    HDU_FIELD(version,TYPE_UINT32,2,false,1)
)

HDU_UNIT_WITH(auth,(HDU_BASE(auth_protocol)),
    HDU_FIELD(content,TYPE_DATAUNIT,3)
)

using AuthManaged=auth::managed;

class WithAuth
{
    public:

        auto authHeader() const
        {
            return m_authHeader;
        }

        void resetAuthHeader()
        {
            m_authHeader->reset();
        }

        template <typename UnitT>
        Error serializeAuthHeader(lib::string_view protocol, uint32_t protocolVersion, common::SharedPtr<UnitT> content,
                                  const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
                                );

    private:

        common::ByteArrayShared m_authHeader;
};

HATN_API_NAMESPACE_END

#endif // HATNAPIAUTHUNIT_H
