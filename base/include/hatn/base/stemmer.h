/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file base/stemmer.h
  *
  * Declares AbstractStemmer -- task-spellcheck.md's "hatn base" interface for word stemming.
  *
  */

/****************************************************************************/

#ifndef HATNSTEMMER_H
#define HATNSTEMMER_H

#include <string>
#include <vector>

#include <hatn/common/stdwrappers.h>

#include <hatn/base/base.h>
#include <hatn/base/spellchecker.h>

HATN_BASE_NAMESPACE_BEGIN

/**
 * @brief Public API for word stemming (task-spellcheck.md).
 *
 * Sits next to AbstractSpellChecker: both are one-word-at-a-time interfaces over the token
 * stream common::AbstractUtf8Proc::tokenizeNormalized() produces. Ships with no implementation
 * of its own -- see makeDefaultStemmer() in spellchecker.h.
 */
class HATN_BASE_EXPORT AbstractStemmer
{
    public:

        AbstractStemmer()=default;
        virtual ~AbstractStemmer();
        AbstractStemmer(const AbstractStemmer&)=default;
        AbstractStemmer(AbstractStemmer&&)=default;
        AbstractStemmer& operator=(const AbstractStemmer&)=default;
        AbstractStemmer& operator=(AbstractStemmer&&)=default;

        //! Stems of `word`, best first. EMPTY when the backend has nothing for it -- a caller
        //! falls back to the word itself (see stemOrSelf()), never to an error.
        virtual std::vector<std::string> stem(const LanguageTag& language, common::lib::string_view word) const =0;

        //! Backend-specific morphological analysis (hunspell's "st:... ts:..." analysis lines).
        //! May be empty even when stem() is not.
        virtual std::vector<std::string> analyze(const LanguageTag& language, common::lib::string_view word) const =0;

        //! First stem, or `word` itself if there is none -- what an indexer actually wants.
        std::string stemOrSelf(const LanguageTag& language, common::lib::string_view word) const;
};

HATN_BASE_NAMESPACE_END

#endif // HATNSTEMMER_H
