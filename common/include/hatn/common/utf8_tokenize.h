/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/utf8_tokenize.h
 *
 *     Strings tokenization and normalization types
 */
/****************************************************************************/

#ifndef HATNUTF8TOKENIZE_H
#define HATNUTF8TOKENIZE_H

#include <string>
#include <vector>

#include <hatn/common/common.h>
#include <hatn/common/stdwrappers.h>

HATN_COMMON_NAMESPACE_BEGIN

class HATN_COMMON_EXPORT AbstractUtf8Proc
{
    public:

        AbstractUtf8Proc()=default;
        virtual ~AbstractUtf8Proc();
        AbstractUtf8Proc(const AbstractUtf8Proc&)=default;
        AbstractUtf8Proc(AbstractUtf8Proc&&) =default;
        AbstractUtf8Proc& operator=(const AbstractUtf8Proc&)=default;
        AbstractUtf8Proc& operator=(AbstractUtf8Proc&&) =default;

        virtual std::string normalize(lib::string_view input) const =0;

        virtual std::vector<std::string> tokenize(lib::string_view normalized) const =0;

        std::vector<std::string> tokenizeNormalized(lib::string_view input) const
        {
            auto normalized=normalize(input);
            return tokenize(normalized);
        }
};

#ifdef HATN_USE_UTF8PROC

class HATN_COMMON_EXPORT Utf8Proc : public AbstractUtf8Proc
{
    public:

        virtual std::string normalize(lib::string_view input) const override;
        virtual std::vector<std::string> tokenize(lib::string_view normalized) const override;
};

#endif

HATN_COMMON_NAMESPACE_END

#endif // HATNUTF8TOKENIZE_H
