/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/message.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIMESSAGE_H
#define HATNAPIMESSAGE_H

#include <hatn/common/result.h>
#include <hatn/common/allocatoronstack.h>
#include <hatn/dataunit/wiredata.h>

#include <hatn/api/api.h>
#include <hatn/api/protocol.h>

HATN_API_NAMESPACE_BEGIN

template <typename BufT=du::WireData>
class Message
{
    public:

        using BufType=BufT;

        Message(const char* typeName=nullptr) : m_typeName(typeName)
        {}

        template <typename UnitT>
        Error setContent(const UnitT& message, const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault(), const char* name=nullptr);

        common::SharedPtr<BufType> content() const
        {
            return m_content;
        }

        common::SpanBuffers chainBuffers(const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()) const
        {
            return m_content->chainBuffers(factory);
        }

        void reset()
        {
            m_content.reset();
        }

        void setTypeName(const char* name) noexcept
        {
            m_typeName=name;
        }

        lib::string_view typeName() const noexcept
        {
            return m_typeName;
        }

    private:

        common::SharedPtr<BufType> m_content;
        const char* m_typeName;
};

HATN_API_NAMESPACE_END

#endif // HATNAPIMESSAGE_H
