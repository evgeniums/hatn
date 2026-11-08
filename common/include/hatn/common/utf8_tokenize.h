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

/**
 * A word of a text together with its place in that text.
 */
struct Utf8Token
{
    std::string token;      //!< the word exactly as it is written in the input (not normalized)
    size_t offset=0;        //!< byte offset of the word in the input
    size_t length=0;        //!< length of the word in bytes
};

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

        virtual std::vector<std::string> tokenizeWithDomains(lib::string_view normalized) const =0;

        /**
         * Split a text into words and report where each word is.
         * @param input Text as it is written, NOT normalized: normalization removes diacritics
         *              (e.g. a Cyrillic "й" becomes "и"), which can make a word unknown to a dictionary,
         *              so a caller stems the raw word first and normalizes the result.
         * @param withDomains If true then "." and "-" between letters or digits stay inside a word,
         *                    as in tokenizeWithDomains(); by default every punctuation mark splits.
         * @return Words in order of appearance. An invalid UTF-8 byte is a boundary.
         *
         * Added after the other virtual methods of this class on purpose.
         */
        virtual std::vector<Utf8Token> tokenizeWithOffsets(lib::string_view input, bool withDomains=false) const =0;

        std::vector<std::string> tokenizeNormalized(lib::string_view input) const
        {
            auto normalized=normalize(input);
            return tokenize(normalized);
        }

        std::vector<std::string> tokenizeNormalizedWithDomains(lib::string_view input) const
        {
            auto normalized=normalize(input);
            return tokenizeWithDomains(normalized);
        }
};

#ifdef HATN_USE_UTF8PROC

class HATN_COMMON_EXPORT Utf8Proc : public AbstractUtf8Proc
{
    public:

        std::string normalize(lib::string_view input) const override;
        std::vector<std::string> tokenize(lib::string_view normalized) const override;
        std::vector<std::string> tokenizeWithDomains(lib::string_view normalized) const override;
        std::vector<Utf8Token> tokenizeWithOffsets(lib::string_view input, bool withDomains=false) const override;
};

#endif

HATN_COMMON_NAMESPACE_END

#endif // HATNUTF8TOKENIZE_H
