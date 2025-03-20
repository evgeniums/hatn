/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/auth.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIAUTH_H
#define HATNAPIAUTH_H

#include <hatn/common/error.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

class Auth
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
                                  int fieldId,
                                  const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
                                  );

        operator bool() const noexcept
        {
            return m_authHeader;
        }

    private:

        common::ByteArrayShared m_authHeader;
};

HATN_API_NAMESPACE_END

#endif // HATNAPIAUTH_H
