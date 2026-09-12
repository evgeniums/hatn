/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file base/hunspellchecker.h
  *
  * Declares HunspellChecker/HunspellStemmer -- the reference hunspell-backed implementation of
  * AbstractSpellChecker/AbstractStemmer (task-spellcheck.md). The whole file is compiled only
  * when HATN_USE_HUNSPELL is defined -- see base/CMakeLists.txt's FIND_PACKAGE(hunspell) block --
  * so a build without the dependency never sees this class at all; makeDefaultSpellChecker()/
  * makeDefaultStemmer() in spellchecker.h are the only place the #ifdef is consulted.
  *
  */

/****************************************************************************/

#ifndef HATNHUNSPELLCHECKER_H
#define HATNHUNSPELLCHECKER_H

#include <hatn/base/config.h>

#ifdef HATN_USE_HUNSPELL

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <hatn/base/base.h>
#include <hatn/base/spellchecker.h>
#include <hatn/base/stemmer.h>

//! Forward declaration only -- the real hunspell/hunspell.hxx is included in
//! src/hunspellchecker.cpp, never in this public header, so a consumer of HunspellChecker's
//! interface does not have to have the hunspell headers on its own include path. Hunspell is
//! declared at GLOBAL scope by the vendor header, not inside any namespace.
class Hunspell;

HATN_BASE_NAMESPACE_BEGIN

class HunspellStemmer;

/**
 * @brief Reference hunspell-backed AbstractSpellChecker (task-spellcheck.md).
 *
 * One Hunspell instance PER LANGUAGE, ordered by load order (or, once set, by
 * activeLanguages()). Hunspell::add_dic() is deliberately NOT used to merge across languages --
 * it loads an extra .dic under the FIRST dictionary's own affix rules, which is right for a
 * domain word list within one language and wrong across languages (a Russian word judged by
 * English affix rules). "Merging depending on the set of languages" (the task's own wording) is
 * therefore the dictionary-VOTE spell()/suggest() already implement in the base interface, not a
 * physical merge -- see AbstractSpellChecker's own doc comment.
 *
 * Personal words (addWord()) and ignored words (ignoreWord()) are plain hash sets consulted
 * BEFORE any Hunspell handle, not fed through Hunspell::add(): they need no affix rules, they
 * work with zero dictionaries loaded, they are trivially enumerable for a caller to persist, and
 * clearIgnored() has no clean Hunspell-side inverse (Hunspell::remove() removes a specific word,
 * not "every word add() was ever called with").
 *
 * Thread safety: a Hunspell instance is not documented as re-entrant, and suggest()/analyze() can
 * take measurable time on a large dictionary. Every public method here is guarded by one mutex --
 * negligible next to hunspell's own work, and what makes this object safe to hand to a worker
 * thread while another thread reads dictionaries().
 */
class HATN_BASE_EXPORT HunspellChecker : public AbstractSpellChecker
{
    public:

        HunspellChecker();
        ~HunspellChecker() override;
        HunspellChecker(const HunspellChecker&)=delete;
        HunspellChecker(HunspellChecker&&)=delete;
        HunspellChecker& operator=(const HunspellChecker&)=delete;
        HunspellChecker& operator=(HunspellChecker&&)=delete;

        Error loadDictionaryFromFile(
            const LanguageTag& language,
            common::lib::string_view dicPath,
            common::lib::string_view affPath=common::lib::string_view{}
        ) override;

        /**
         * @brief See AbstractSpellChecker::loadDictionaryFromBuffer(). Hunspell has NO in-memory
         *  constructor -- only the (affpath,dpath) file-path one above -- so this spills `dic`/
         *  `aff` into two files under a unique name in the system temp directory, constructs a
         *  Hunspell instance against them (which reads both files fully during construction),
         *  then removes them immediately on every platform except Windows, where an open file
         *  cannot be unlinked -- there the temp files are removed from this object's destructor
         *  instead. A backend LIMITATION, not a design choice: documented here so it is not
         *  mistaken for one later.
         */
        Error loadDictionaryFromBuffer(
            const LanguageTag& language,
            common::lib::string_view dic,
            common::lib::string_view aff=common::lib::string_view{}
        ) override;

        Error loadWordList(const LanguageTag& language, common::lib::string_view words) override;

        void unloadDictionary(const LanguageTag& language) override;

        void unloadAll() override;

        std::vector<DictionaryInfo> dictionaries() const override;

        void setActiveLanguages(std::vector<LanguageTag> languages) override;

        const std::vector<LanguageTag>& activeLanguages() const override;

        Error addWord(const LanguageTag& language, common::lib::string_view word) override;

        void ignoreWord(common::lib::string_view word) override;

        void clearIgnored() override;

        bool spell(common::lib::string_view word) const override;

        LanguageTag spellLanguage(common::lib::string_view word) const override;

        std::vector<std::string> suggest(common::lib::string_view word, size_t maxCount=8) const override;

    private:

        friend class HunspellStemmer;

        struct Dict
        {
            LanguageTag language;

            //! The path/buffer name this dictionary was loaded under -- diagnostics only.
            std::string name;

            //! Never null once constructed into m_dicts.
            std::unique_ptr<Hunspell> handle;

            //! Set only by loadDictionaryFromBuffer() -- the temp files to remove in the dtor on
            //! Windows (see that method's own doc comment). Empty everywhere else.
            std::string tempDicPath;
            std::string tempAffPath;
        };

        //! Languages to consult, in order: activeLanguages() if set, else every loaded
        //! dictionary in LOAD order. Callers already hold m_mutex.
        std::vector<LanguageTag> iterationOrderLocked() const;

        //! nullptr if `language` has no loaded dictionary. Callers already hold m_mutex.
        Dict* dictForLocked(const LanguageTag& language);
        const Dict* dictForLocked(const LanguageTag& language) const;

        //! Personal word OR global ("") bucket contains `word`. Callers already hold m_mutex.
        bool isPersonalWordLocked(const LanguageTag& language, const std::string& word) const;

        mutable std::mutex m_mutex;
        std::vector<Dict> m_dicts;
        std::vector<LanguageTag> m_activeLanguages;

        //! Keyed by language, "" (the global bucket) applies to every language -- see
        //! AbstractSpellChecker::loadWordList()'s own doc comment.
        std::unordered_map<LanguageTag,std::unordered_set<std::string>> m_personalWords;

        std::unordered_set<std::string> m_ignoredWords;
};

/**
 * @brief Reference hunspell-backed AbstractStemmer, sharing an EXISTING HunspellChecker's
 *  dictionaries rather than loading its own -- see makeDefaultStemmer()'s own doc comment in
 *  spellchecker.h for why that sharing is not optional.
 */
class HATN_BASE_EXPORT HunspellStemmer : public AbstractStemmer
{
    public:

        explicit HunspellStemmer(std::shared_ptr<HunspellChecker> checker);

        std::vector<std::string> stem(const LanguageTag& language, common::lib::string_view word) const override;

        std::vector<std::string> analyze(const LanguageTag& language, common::lib::string_view word) const override;

    private:

        std::shared_ptr<HunspellChecker> m_checker;
};

HATN_BASE_NAMESPACE_END

#endif // HATN_USE_HUNSPELL

#endif // HATNHUNSPELLCHECKER_H
