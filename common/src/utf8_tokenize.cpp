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
    utf8proc_ssize_t size = static_cast<utf8proc_ssize_t>(normalized.size());
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

//---------------------------------------------------------------

namespace {

bool is_alnum(utf8proc_category_t cat)
{
    return (cat >= UTF8PROC_CATEGORY_LU && cat <= UTF8PROC_CATEGORY_LO) || // Letters
           (cat == UTF8PROC_CATEGORY_ND);                                 // Numbers
}

}

std::vector<std::string> Utf8Proc::tokenizeWithDomains(std::string_view normalized) const
{
    std::vector<std::string> tokens;
    std::string current_token;

    const utf8proc_uint8_t* ptr = (const utf8proc_uint8_t*)normalized.data();
    utf8proc_ssize_t size = static_cast<utf8proc_ssize_t>(normalized.size());

    utf8proc_int32_t cp;
    utf8proc_ssize_t n;
    utf8proc_category_t prev_cat = UTF8PROC_CATEGORY_CN; // "Unassigned" initially

    while (size > 0 && (n = utf8proc_iterate(ptr, size, &cp)) > 0) {
        utf8proc_category_t cat = utf8proc_category(cp);

        bool is_boundary = false;

        // 1. Check for standard separators/controls
        if ((cat >= UTF8PROC_CATEGORY_ZS && cat <= UTF8PROC_CATEGORY_ZP) || cat == UTF8PROC_CATEGORY_CC || cat == UTF8PROC_CATEGORY_ZL) {
            is_boundary = true;
        }
        // 2. Specialized Punctuation Handling (The "Domain Glue" logic)
        else if (cat >= UTF8PROC_CATEGORY_PC && cat <= UTF8PROC_CATEGORY_PO) {
            bool is_glue = false;
            if (cp == '.' || cp == '-') {
                // Peek at next character
                const utf8proc_uint8_t* next_ptr = ptr + n;
                utf8proc_ssize_t next_size = size - n;
                utf8proc_int32_t next_cp;
                if (next_size > 0 && utf8proc_iterate(next_ptr, next_size, &next_cp) > 0) {
                    // It's "glue" if surrounded by Alphanumerics
                    if (is_alnum(prev_cat) && is_alnum(utf8proc_category(next_cp))) {
                        is_glue = true;
                    }
                }
            }
            if (!is_glue) is_boundary = true;
        }

        if (is_boundary) {
            if (!current_token.empty()) {
                tokens.push_back(std::move(current_token));
                current_token.clear();
            }
            prev_cat = UTF8PROC_CATEGORY_CN;
        } else {
            utf8proc_uint8_t buf[4];
            utf8proc_ssize_t len = utf8proc_encode_char(cp, buf);
            current_token.append((char*)buf, len);
            prev_cat = cat;
        }

        ptr += n;
        size -= n;
    }

    if (!current_token.empty()) tokens.push_back(std::move(current_token));
    return tokens;
}

//---------------------------------------------------------------

std::vector<Utf8Token> Utf8Proc::tokenizeWithOffsets(std::string_view input, bool withDomains) const
{
    std::vector<Utf8Token> tokens;

    const auto* data=reinterpret_cast<const utf8proc_uint8_t*>(input.data());
    const auto total=static_cast<utf8proc_ssize_t>(input.size());

    utf8proc_ssize_t pos=0;
    utf8proc_ssize_t start=-1;
    utf8proc_category_t prevCat=UTF8PROC_CATEGORY_CN;

    auto flush=[&](utf8proc_ssize_t end)
    {
        if (start>=0)
        {
            Utf8Token token;
            token.offset=static_cast<size_t>(start);
            token.length=static_cast<size_t>(end-start);
            token.token.assign(input.data()+start,token.length);
            tokens.push_back(std::move(token));
            start=-1;
        }
        prevCat=UTF8PROC_CATEGORY_CN;
    };

    while (pos<total)
    {
        utf8proc_int32_t cp=0;
        auto n=utf8proc_iterate(data+pos,total-pos,&cp);
        if (n<=0)
        {
            // invalid byte: a boundary, skip it
            flush(pos);
            ++pos;
            continue;
        }

        auto cat=utf8proc_category(cp);
        bool boundary=false;

        if (cat==UTF8PROC_CATEGORY_ZS || cat==UTF8PROC_CATEGORY_ZL ||
            cat==UTF8PROC_CATEGORY_ZP || cat==UTF8PROC_CATEGORY_CC)
        {
            boundary=true;
        }
        else if (cat>=UTF8PROC_CATEGORY_PC && cat<=UTF8PROC_CATEGORY_PO)
        {
            boundary=true;
            if (withDomains && (cp=='.' || cp=='-') && start>=0 && is_alnum(prevCat))
            {
                utf8proc_int32_t nextCp=0;
                if (pos+n<total && utf8proc_iterate(data+pos+n,total-pos-n,&nextCp)>0
                    && is_alnum(utf8proc_category(nextCp)))
                {
                    boundary=false;
                }
            }
        }

        if (boundary)
        {
            flush(pos);
        }
        else
        {
            if (start<0)
            {
                start=pos;
            }
            prevCat=cat;
        }
        pos+=n;
    }
    flush(pos);

    return tokens;
}

#endif

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END
