/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file base/languagedetector.h
  *
  * Declares AbstractLanguageDetector -- task-spellcheck.md's "hatn base" interface for language
  * auto-detection.
  *
  * INTERFACE ONLY in this change: no implementation and no third-party dependency (no cld3, no
  * protobuf) ships with it. Spell checking uses the dictionary-vote rule
  * (AbstractSpellChecker::spell()) and needs no detector at all -- a word is misspelled only when
  * NO active dictionary accepts it. A detector backend is deferred to a later change, where its
  * only job is to decide which dictionaries to auto-download for a newly detected language, never
  * to decide whether a word is correctly spelled. See spellchecker.h's makeDefaultLanguageDetector(),
  * which always returns nullptr in this build.
  *
  */

/****************************************************************************/

#ifndef HATNLANGUAGEDETECTOR_H
#define HATNLANGUAGEDETECTOR_H

#include <string>
#include <vector>

#include <hatn/common/stdwrappers.h>

#include <hatn/base/base.h>
#include <hatn/base/spellchecker.h>

HATN_BASE_NAMESPACE_BEGIN

//! One candidate language and how confident the backend is in it.
struct LanguageProbability
{
    LanguageTag language;

    //! 0..1.
    float probability=0.0f;

    //! The backend's OWN confidence flag -- not merely a threshold applied to `probability`
    //! above, since a backend may reserve this for e.g. "text was long enough to trust at all".
    bool isReliable=false;
};

/**
 * @brief Public API for language auto-detection (task-spellcheck.md). Interface only -- see this
 *  file's own header comment for why no implementation ships with it in this change.
 */
class HATN_BASE_EXPORT AbstractLanguageDetector
{
    public:

        AbstractLanguageDetector()=default;
        virtual ~AbstractLanguageDetector();
        AbstractLanguageDetector(const AbstractLanguageDetector&)=default;
        AbstractLanguageDetector(AbstractLanguageDetector&&)=default;
        AbstractLanguageDetector& operator=(const AbstractLanguageDetector&)=default;
        AbstractLanguageDetector& operator=(AbstractLanguageDetector&&)=default;

        //! Best guess, or an empty tag if the backend has no opinion at all.
        virtual LanguageTag detect(common::lib::string_view text) const =0;

        virtual std::vector<LanguageProbability> detectAll(common::lib::string_view text, size_t maxCount=3) const =0;

        //! Shortest text this backend can say anything useful about. 0 means no opinion --
        //! the backend answers regardless of length.
        virtual size_t minTextLength() const noexcept
        {
            return 0;
        }
};

HATN_BASE_NAMESPACE_END

#endif // HATNLANGUAGEDETECTOR_H
