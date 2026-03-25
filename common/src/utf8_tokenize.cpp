/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/utf8_tokenize.cpp
 *
 *  normalize() and tokenize() methods are written with help of Google AI
 */
/****************************************************************************/

#include <utf8proc.h>

#include <hatn/common/utf8_tokenize.h>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------

AbstractUtf8Proc::~AbstractUtf8Proc()
{}

#ifdef HATN_USE_UTF8PROC

//---------------------------------------------------------------

std::string Utf8Proc::normalize(lib::string_view input) const
{
    utf8proc_uint8_t *output;

    // Flags:
    // - CASEFOLD: Better than lowercase for search (handles 'ß', etc.)
    // - COMPOSE: Returns normalized NFC form
    // - STRIPMARK: Removes diacritics (accents)
    // - STABLE: Ensures consistent results across versions
    auto flags = UTF8PROC_CASEFOLD | UTF8PROC_COMPOSE | UTF8PROC_STRIPMARK | UTF8PROC_STABLE;
    utf8proc_ssize_t result = utf8proc_map(
        (const utf8proc_uint8_t*)input.data(),
        input.size(),
        &output,
        static_cast<utf8proc_option_t>(static_cast<int>(flags)) // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
    );

    if (result < 0) return "";

    std::string normalized((char*)output);
    free(output);
    return normalized;
}

//---------------------------------------------------------------

std::vector<std::string> Utf8Proc::tokenize(std::string_view normalized) const
{
    std::vector<std::string> tokens;
    std::string current_token;

    const utf8proc_uint8_t* ptr = (const utf8proc_uint8_t*)normalized.data();
    ssize_t size = static_cast<ssize_t>(normalized.size());
    utf8proc_int32_t codepoint;
    utf8proc_ssize_t n;

    while (size > 0 && (n = utf8proc_iterate(ptr, size, &codepoint)) > 0)
    {
        utf8proc_category_t cat = utf8proc_category(codepoint);

        // Comprehensive boundaries for indexing:
        // Z* = Any Separator (Space, Line, Paragraph)
        // Cc = Control characters (\n, \r, \t)
        // P* = Any Punctuation
        if (cat == UTF8PROC_CATEGORY_ZS || cat == UTF8PROC_CATEGORY_ZL ||
            cat == UTF8PROC_CATEGORY_ZP || cat == UTF8PROC_CATEGORY_CC ||
            (cat >= UTF8PROC_CATEGORY_PC && cat <= UTF8PROC_CATEGORY_PO))
        {
            if (!current_token.empty())
            {
                tokens.push_back(std::move(current_token));
                current_token.clear();
            }
        }
        else
        {
            utf8proc_uint8_t buf[4];
            utf8proc_ssize_t len = utf8proc_encode_char(codepoint, buf);
            current_token.append((char*)buf, len);
        }
        ptr += n;
        size -= n;
    }

    if (!current_token.empty()) tokens.push_back(std::move(current_token));
    return tokens;
}

#endif

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END
