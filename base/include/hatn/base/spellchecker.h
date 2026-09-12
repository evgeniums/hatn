/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file base/spellchecker.h
  *
  * Declares AbstractSpellChecker -- task-spellcheck.md's "hatn base" interface for spell
  * checking and dictionary management.
  *
  */

/****************************************************************************/

#ifndef HATNSPELLCHECKER_H
#define HATNSPELLCHECKER_H

#include <memory>
#include <string>
#include <vector>

#include <hatn/common/stdwrappers.h>
#include <hatn/common/error.h>

#include <hatn/base/base.h>

HATN_BASE_NAMESPACE_BEGIN

class AbstractStemmer;
class AbstractLanguageDetector;

//! Language tag as the DICTIONARY names itself -- "en_US", "ru_RU", "de_DE". A key, not a
//! locale: this layer never parses it, so a backend with its own naming convention is free to
//! use it verbatim.
using LanguageTag=std::string;

//! One loaded dictionary, for AbstractSpellChecker::dictionaries().
struct DictionaryInfo
{
    LanguageTag language;

    //! File or buffer name it was loaded under -- diagnostics only, not a lookup key.
    std::string name;

    //! A user's own word list (AbstractSpellChecker::loadWordList()) rather than a shipped
    //! dictionary.
    bool isPersonal=false;
};

/**
 * @brief Public API for spell checking and dictionary management (task-spellcheck.md).
 *
 * Ships with no dictionary of its own -- see makeDefaultSpellChecker() below, which returns the
 * hunspell-backed HunspellChecker when the optional dependency was found at configure time
 * (HATN_USE_HUNSPELL), and nullptr otherwise. This is the ONLY place that define is consulted; no
 * consumer of this interface writes the #ifdef.
 *
 * Tokenizing free text into words is deliberately NOT this interface's job:
 * common::AbstractUtf8Proc::tokenizeNormalized() already produces the token stream this
 * interface (and AbstractStemmer) is meant to consume one word at a time.
 *
 * "Merging dictionaries depending on the set of languages" (the task's own wording) is
 * setActiveLanguages() plus a VOTE computed per call in spell()/suggest() -- not a physical merge
 * of dictionary files. A word is misspelled only when NO active dictionary accepts it, which is
 * exactly why spell checking needs no language detector at all: a detector is useful only to
 * decide which dictionaries to fetch, never to decide whether a word is correct.
 *
 * A Qt-facing, synchronous, tri-state seam sits in front of an implementation of this interface
 * in uise-desktop (uise::AbstractSpellChecker) -- that adapter, and the worker thread it needs,
 * are whitemdesktop's responsibility and do not exist in this codebase yet; this interface itself
 * is Qt-free and may block.
 */
class HATN_BASE_EXPORT AbstractSpellChecker
{
    public:

        AbstractSpellChecker()=default;
        virtual ~AbstractSpellChecker();
        AbstractSpellChecker(const AbstractSpellChecker&)=default;
        AbstractSpellChecker(AbstractSpellChecker&&)=default;
        AbstractSpellChecker& operator=(const AbstractSpellChecker&)=default;
        AbstractSpellChecker& operator=(AbstractSpellChecker&&)=default;

        // ---- dictionary management ---------------------------------------------------------

        /**
         * @brief Load a dictionary for `language` from files.
         * @param affPath May be empty for a backend that needs no affix file.
         *
         * Replacing an already-loaded language is an ERROR (BaseError::SPELL_DICTIONARY_EXISTS)
         * -- call unloadDictionary() first, so a silent half-swap (old dictionary partially
         * replaced mid-load) is impossible.
         */
        virtual Error loadDictionaryFromFile(
            const LanguageTag& language,
            common::lib::string_view dicPath,
            common::lib::string_view affPath=common::lib::string_view{}
        ) =0;

        /**
         * @brief The same, from memory: `dic`/`aff` are the FILE CONTENTS, not paths.
         *
         * For a dictionary that arrived over the network, out of an archive, or out of a
         * database blob, without the caller having to materialize a file for it first.
         */
        virtual Error loadDictionaryFromBuffer(
            const LanguageTag& language,
            common::lib::string_view dic,
            common::lib::string_view aff=common::lib::string_view{}
        ) =0;

        /**
         * @brief Add a newline-separated plain word list -- a personal/custom dictionary.
         * @param language Empty means the list applies to EVERY language.
         *
         * Needs no affix rules, so it is a separate call rather than a flavour of
         * loadDictionaryFromBuffer() that happens to be given no `aff`.
         */
        virtual Error loadWordList(const LanguageTag& language, common::lib::string_view words) =0;

        virtual void unloadDictionary(const LanguageTag& language) =0;

        virtual void unloadAll() =0;

        virtual std::vector<DictionaryInfo> dictionaries() const =0;

        /**
         * @brief Languages consulted by spell()/suggest(). EMPTY means "every loaded dictionary".
         *
         * This is the whole of "merging depending on the set of languages" -- see this class's
         * own doc comment.
         */
        virtual void setActiveLanguages(std::vector<LanguageTag> languages) =0;

        virtual const std::vector<LanguageTag>& activeLanguages() const =0;

        /**
         * @brief Add one word to the in-memory personal dictionary.
         *
         * NOT persisted by this layer -- persistence belongs to whoever owns the checker
         * (whitemclient, per this task's own project split).
         */
        virtual Error addWord(const LanguageTag& language, common::lib::string_view word) =0;

        /**
         * @brief Session-only suppression, cleared by clearIgnored().
         *
         * Distinct from addWord(): an ignored word is not offered back through dictionaries()
         * and is not a candidate for persisting -- "stop underlining this, just for now".
         */
        virtual void ignoreWord(common::lib::string_view word) =0;

        virtual void clearIgnored() =0;

        // ---- checking -------------------------------------------------------------------------

        /**
         * @brief true when ANY active dictionary accepts `word` -- the dictionary-vote rule.
         *
         * A word is misspelled only when NO active dictionary accepts it, which is what makes a
         * language detector unnecessary for spell checking itself (see this class's own doc
         * comment).
         */
        virtual bool spell(common::lib::string_view word) const =0;

        //! Which active language accepted `word`, or an empty tag if none did. Ordered by
        //! activeLanguages(), so the answer is stable rather than dictionary-insertion-ordered.
        virtual LanguageTag spellLanguage(common::lib::string_view word) const =0;

        //! Replacements, best first, merged across active languages, de-duplicated, capped at
        //! `maxCount`.
        virtual std::vector<std::string> suggest(common::lib::string_view word, size_t maxCount=8) const =0;

        // ---- non-virtual convenience ------------------------------------------------------

        //! Whether this checker has any dictionary loaded at all.
        bool isReady() const
        {
            return !dictionaries().empty();
        }

        bool hasLanguage(const LanguageTag& language) const;
};

/**
 * @brief Spell checker for this build, or nullptr when no backend was compiled in
 *  (HATN_USE_HUNSPELL not defined). The only place that define is consulted -- see
 *  AbstractSpellChecker's own doc comment.
 */
HATN_BASE_EXPORT std::shared_ptr<AbstractSpellChecker> makeDefaultSpellChecker();

/**
 * @brief Stemmer for this build, sharing `checker`'s already-loaded dictionaries, or nullptr
 *  when no backend was compiled in OR `checker` was not produced by makeDefaultSpellChecker().
 * @param checker A checker returned by makeDefaultSpellChecker() -- REQUIRED, not optional:
 *  hunspell's own dictionary object holds both the spelling data and the affix rules stemming
 *  needs, so wrapping the checker's existing handles is what avoids loading every dictionary
 *  TWICE (once per interface) for no gain. There is no "stemmer with no dictionaries of its own"
 *  -- get one from a checker that already has some loaded.
 */
HATN_BASE_EXPORT std::shared_ptr<AbstractStemmer> makeDefaultStemmer(
    const std::shared_ptr<AbstractSpellChecker>& checker
);

/**
 * @brief ALWAYS nullptr in this build: no language-detection backend is compiled in
 *  (task-spellcheck.md, explicit scope decision -- spell checking needs the dictionary-vote
 *  rule, not a detector; a detector backend is deferred to a later change, to drive dictionary
 *  auto-download rather than to decide misspellings).
 *
 * Declared now so a consumer already writes the null check, and landing a real backend later is
 * a one-line change inside this function, not an API change.
 */
HATN_BASE_EXPORT std::shared_ptr<AbstractLanguageDetector> makeDefaultLanguageDetector();

HATN_BASE_NAMESPACE_END

#endif // HATNSPELLCHECKER_H
