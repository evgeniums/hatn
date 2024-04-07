/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/errorstack.h
  *
  *     Contains definition of native error representing a stack of errors.
  *
  */

/****************************************************************************/

#ifndef HATNERRORSTACK_H
#define HATNERRORSTACK_H

#include <hatn/common/format.h>

#include <hatn/common/error.h>
#include <hatn/common/apierror.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Stack of errors.
class HATN_COMMON_EXPORT ErrorStack
{
    public:

        void reset() noexcept
        {
            m_stack.clear();
            resetState();
        }

        void resetState() noexcept
        {
            m_index=static_cast<int>(m_stack.size())-1;
        }

        const Error* next() const noexcept
        {
            if (m_index<0)
            {
                return nullptr;
            }
            return &(m_stack.at(static_cast<size_t>(m_index--)));
        }

        bool isNull() const noexcept
        {
            return m_stack.empty();
        }

        operator bool() const noexcept
        {
            return !isNull();
        }

        std::string message() const
        {
            if (isNull())
            {
                return std::string();
            }

            std::string msg=m_stack.at(m_stack.size()-1).message();
            for (size_t i=m_stack.size()-2;i>0;i--)
            {
                auto nextMsg=m_stack.at(i).message();
                if (!msg.empty())
                {
                    if (!nextMsg.empty())
                    {
                        msg=fmt::format("{}: {}",msg,nextMsg);
                    }
                }
                else
                {
                    msg=std::move(nextMsg);
                }
            }
            return msg;
        }

        const ApiError* apiError() const noexcept
        {
            for (size_t i=m_stack.size()-1;i>0;i--)
            {
                auto apiError=m_stack.at(i).apiError();
                if (apiError!=nullptr)
                {
                    return apiError;
                }
            }
            return nullptr;
        }

        void push(Error error)
        {
            m_stack.emplace_back(std::move(error));
            resetState();
        }

        void reserve(size_t n)
        {
            m_stack.reserve(n);
        }

    private:

        std::vector<Error> m_stack;
        mutable int m_index=-1;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNERRORSTACK_H
