/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/clientresponse.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTRESPONSE_H
#define HATNAPICLIENTRESPONSE_H

#include <hatn/dataunit/unitwrapper.h>

#include <hatn/api/api.h>
#include <hatn/api/responseunit.h>
#include <hatn/api/makeapierror.h>
#include <hatn/api/apiliberror.h>
#include <hatn/api/genericerror.h>

HATN_API_NAMESPACE_BEGIN

namespace client
{

class StreamChannel;

class Response
{
    public:

        using IdType=std::string;

        Response()=default;        

        void setMessageData(common::ByteArrayShared value)
        {
            m_messageData=std::move(value);
        }

        auto messageData() const noexcept
        {
            return m_messageData;
        }

        void setStatus(protocol::ResponseStatus value) noexcept
        {
            m_status=value;
        }

        auto status() const noexcept
        {
            return m_status;
        }

        void setMessageType(std::string value) noexcept
        {
            m_messageType=std::move(value);
        }

        const std::string& messageType() const noexcept
        {
            return m_messageType;
        }

        void setId(IdType value) noexcept
        {
            m_id=std::move(value);
        }

        const IdType& id() const noexcept
        {
            return m_id;
        }

        bool isSuccess() const noexcept
        {
            return status()==protocol::ResponseStatus::Success;
        }

        template <typename UnitT>
        Error parse(UnitT& obj, lib::string_view messageType={}) const
        {
            // check message type
            if (messageType.empty())
            {
                messageType=obj.unitName();
            }
            if (messageType != m_messageType)
            {
                return apiLibError(ApiLibError::MISMATCHED_RESPONSE_MESSAGE_TYPE);
            }

            // deserialize message
            du::WireBufSolidShared buf{m_messageData};
            Error ec;
            if (!du::io::deserialize(obj,buf))
            {
                return apiLibError(ApiLibError::FAILED_DESERIALIZE_RESPONSE_ERROR);
            }

            // done
            return OK;
        }

        void setErrror(Error ec)
        {
            m_error=std::move(ec);
        }

        Error error() const
        {
            return m_error;
        }

        void setStreamChannel(std::shared_ptr<StreamChannel> channel)
        {
            m_channel=std::move(channel);
        }

        std::shared_ptr<StreamChannel> streamChannel() const
        {
            return m_channel;
        }

    private:

        IdType m_id;
        protocol::ResponseStatus m_status;
        std::string m_messageType;
        common::ByteArrayShared m_messageData;

        Error m_error;

        std::shared_ptr<StreamChannel> m_channel;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTRESPONSE_H
