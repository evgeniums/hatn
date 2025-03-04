/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/env.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERENV_H
#define HATNAPISERVERENV_H

#include <hatn/api/api.h>
#include <hatn/api/tenancy.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

class SimpleEnv
{
    public:
};

template <typename EnvT=SimpleEnv>
class WithEnv
{
    public:

        void setEnv(common::SharedPtr<EnvT> env)
        {
            m_env=std::move(env);
        }

        common::SharedPtr<EnvT> envShared() const noexcept
        {
            return m_env;
        }

        EnvT* env() const noexcept
        {
            return m_env;
        }

    private:

        common::SharedPtr<EnvT> m_env;
};

} // namespace server

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::server::WithEnv<HATN_API_NAMESPACE::server::SimpleEnv>,HATN_API_EXPORT)

#endif // HATNAPISERVERCONTEXT_H

