/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/modelswrapper.h
  *
  */

/****************************************************************************/

#ifndef HATNDBMODELSWRAPPER_H
#define HATNDBMODELSWRAPPER_H

#include <hatn/db/db.h>
#include <hatn/db/model.h>

HATN_DB_NAMESPACE_BEGIN

class HATN_DB_EXPORT ModelsWrapper
{
    public:

        ModelsWrapper(std::string prefix={}) : m_prefix(std::move(prefix))
        {}

        virtual ~ModelsWrapper();
        ModelsWrapper(const ModelsWrapper&)=default;
        ModelsWrapper(ModelsWrapper&&)=default;
        ModelsWrapper& operator=(const ModelsWrapper&)=default;
        ModelsWrapper& operator=(ModelsWrapper&&)=default;

        std::string prefix() const
        {
            return m_prefix;
        }

    private:

        std::string m_prefix;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBMODELSWRAPPER_H
