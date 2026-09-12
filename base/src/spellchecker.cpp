/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file base/spellchecker.cpp
  *
  * Defines AbstractSpellChecker and the makeDefault*() factories.
  *
  */

/****************************************************************************/

#include <algorithm>

#include <hatn/base/spellchecker.h>
#include <hatn/base/stemmer.h>
#include <hatn/base/languagedetector.h>

#ifdef HATN_USE_HUNSPELL
#include <hatn/base/hunspellchecker.h>
#endif

HATN_BASE_NAMESPACE_BEGIN

//---------------------------------------------------------------

AbstractSpellChecker::~AbstractSpellChecker()
{}

//---------------------------------------------------------------

bool AbstractSpellChecker::hasLanguage(const LanguageTag& language) const
{
    auto dicts=dictionaries();
    return std::any_of(dicts.begin(),dicts.end(),
        [&language](const DictionaryInfo& info)
        {
            return info.language==language;
        }
    );
}

//---------------------------------------------------------------

std::shared_ptr<AbstractSpellChecker> makeDefaultSpellChecker()
{
#ifdef HATN_USE_HUNSPELL
    return std::make_shared<HunspellChecker>();
#else
    return std::shared_ptr<AbstractSpellChecker>{};
#endif
}

//---------------------------------------------------------------

std::shared_ptr<AbstractStemmer> makeDefaultStemmer(const std::shared_ptr<AbstractSpellChecker>& checker)
{
#ifdef HATN_USE_HUNSPELL
    auto hunspellChecker=std::dynamic_pointer_cast<HunspellChecker>(checker);
    if (!hunspellChecker)
    {
        return std::shared_ptr<AbstractStemmer>{};
    }
    return std::make_shared<HunspellStemmer>(std::move(hunspellChecker));
#else
    (void)checker;
    return std::shared_ptr<AbstractStemmer>{};
#endif
}

//---------------------------------------------------------------

std::shared_ptr<AbstractLanguageDetector> makeDefaultLanguageDetector()
{
    // Always nullptr in this build -- see languagedetector.h's own header comment.
    return std::shared_ptr<AbstractLanguageDetector>{};
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
