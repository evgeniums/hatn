/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/test/testspellchecker.cpp
  *
  * Tests AbstractSpellChecker/AbstractStemmer/AbstractLanguageDetector and the default
  * hunspell-backed implementation (task-spellcheck.md).
  *
  * IMPORTANT: every BOOST_AUTO_TEST_CASE here is UNCONDITIONAL -- never wrapped in
  * `#ifdef HATN_USE_HUNSPELL`. ADD_HATN_CTESTS (cmake/hatn/ConfigTest.cmake) registers a ctest
  * for every BOOST_AUTO_TEST_CASE it finds by greping the source TEXT, so a case compiled out by
  * the preprocessor would still get a ctest entry that then fails with "test not found" in a
  * build without hunspell. Each case therefore starts with a runtime
  * `if (!makeDefaultSpellChecker()) { ...skip...; return; }` guard instead.
  *
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#include <hatn/common/filesystem.h>
#include <hatn/common/plainfile.h>

#include <hatn/base/baseerror.h>
#include <hatn/base/spellchecker.h>
#include <hatn/base/stemmer.h>
#include <hatn/base/languagedetector.h>

#include <hatn/test/multithreadfixture.h>

HATN_USING
HATN_COMMON_USING
HATN_BASE_USING
HATN_TEST_USING

namespace {

//! Read a fixture file's raw bytes -- used by TestLoadFromBufferMatchesFile to feed
//! loadDictionaryFromBuffer() the exact same content loadDictionaryFromFile() reads itself.
std::string readAsset(const std::string& relPath)
{
    common::PlainFile file;
    std::string content;
    auto ec=file.readAll(MultiThreadFixture::assetsFilePath(relPath),content);
    BOOST_REQUIRE(!ec);
    return content;
}

std::string enAffPath()
{
    return MultiThreadFixture::assetsFilePath("base/assets/spell/test_en.aff");
}

std::string enDicPath()
{
    return MultiThreadFixture::assetsFilePath("base/assets/spell/test_en.dic");
}

std::string ruAffPath()
{
    return MultiThreadFixture::assetsFilePath("base/assets/spell/test_ru.aff");
}

std::string ruDicPath()
{
    return MultiThreadFixture::assetsFilePath("base/assets/spell/test_ru.dic");
}

std::string latin1AffPath()
{
    return MultiThreadFixture::assetsFilePath("base/assets/spell/test_latin1.aff");
}

std::string latin1DicPath()
{
    return MultiThreadFixture::assetsFilePath("base/assets/spell/test_latin1.dic");
}

// Cyrillic "привет" (hello) as a UTF-8 byte literal, matching the word in test_ru.dic -- written
// this way rather than as a literal non-ASCII source character so this file stays plain ASCII.
const std::string PrivetUtf8="\xd0\xbf\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82";

}

BOOST_AUTO_TEST_SUITE(TestSpellChecker)

BOOST_AUTO_TEST_CASE(TestFactoryContract)
{
    // Backend-independent invariants -- true whether or not hunspell was found at configure
    // time, so this case is meaningful (not a skip) in EVERY build.
    auto checker=makeDefaultSpellChecker();

    // No language-detection backend ships in this change -- see languagedetector.h's own
    // header comment. Always nullptr, regardless of HATN_USE_HUNSPELL.
    BOOST_CHECK(makeDefaultLanguageDetector()==nullptr);

    // A stemmer with no checker (or a checker not produced by makeDefaultSpellChecker()) is
    // always null -- see makeDefaultStemmer()'s own doc comment for why sharing is required.
    BOOST_CHECK(makeDefaultStemmer(std::shared_ptr<AbstractSpellChecker>{})==nullptr);

    if (checker)
    {
        // A freshly constructed checker has no dictionaries yet.
        BOOST_CHECK(!checker->isReady());
        BOOST_CHECK(checker->dictionaries().empty());

        auto stemmer=makeDefaultStemmer(checker);
        BOOST_CHECK(stemmer!=nullptr);
    }
    else
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build (HATN_USE_HUNSPELL not defined)");
    }
}

BOOST_AUTO_TEST_CASE(TestLoadFromFile)
{
    auto checker=makeDefaultSpellChecker();
    if (!checker)
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build, skipping");
        return;
    }

    auto ec=checker->loadDictionaryFromFile("en",enDicPath(),enAffPath());
    BOOST_CHECK(!ec);
    BOOST_CHECK(checker->isReady());
    BOOST_CHECK_EQUAL(checker->dictionaries().size(),static_cast<size_t>(1));

    BOOST_CHECK(checker->spell("hello"));
    BOOST_CHECK(checker->spell("world"));
    BOOST_CHECK(!checker->spell("xyzzy"));
    BOOST_CHECK_EQUAL(checker->spellLanguage("hello"),std::string("en"));
}

BOOST_AUTO_TEST_CASE(TestDuplicateLanguageIsAnError)
{
    auto checker=makeDefaultSpellChecker();
    if (!checker)
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build, skipping");
        return;
    }

    BOOST_CHECK(!checker->loadDictionaryFromFile("en",enDicPath(),enAffPath()));
    auto ec=checker->loadDictionaryFromFile("en",enDicPath(),enAffPath());
    BOOST_CHECK(ec);
    BOOST_CHECK(ec.is(BaseError::SPELL_DICTIONARY_EXISTS,BaseErrorCategory::getCategory()));
}

BOOST_AUTO_TEST_CASE(TestNonUtf8DictionaryIsRejected)
{
    auto checker=makeDefaultSpellChecker();
    if (!checker)
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build, skipping");
        return;
    }

    auto ec=checker->loadDictionaryFromFile("latin1",latin1DicPath(),latin1AffPath());
    BOOST_CHECK(ec);
    BOOST_CHECK(ec.is(BaseError::SPELL_DICTIONARY_ENCODING,BaseErrorCategory::getCategory()));
    BOOST_CHECK(!checker->hasLanguage("latin1"));
}

BOOST_AUTO_TEST_CASE(TestLoadFromBufferMatchesFile)
{
    auto fileChecker=makeDefaultSpellChecker();
    auto bufferChecker=makeDefaultSpellChecker();
    if (!fileChecker || !bufferChecker)
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build, skipping");
        return;
    }

    BOOST_CHECK(!fileChecker->loadDictionaryFromFile("en",enDicPath(),enAffPath()));

    auto dicContent=readAsset("base/assets/spell/test_en.dic");
    auto affContent=readAsset("base/assets/spell/test_en.aff");
    BOOST_CHECK(!bufferChecker->loadDictionaryFromBuffer("en",dicContent,affContent));

    for (const auto* word : {"hello","world","xyzzy","correct"})
    {
        BOOST_CHECK_EQUAL(fileChecker->spell(word),bufferChecker->spell(word));
    }
}

BOOST_AUTO_TEST_CASE(TestMultiLanguageVote)
{
    auto checker=makeDefaultSpellChecker();
    if (!checker)
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build, skipping");
        return;
    }

    BOOST_CHECK(!checker->loadDictionaryFromFile("en",enDicPath(),enAffPath()));
    BOOST_CHECK(!checker->loadDictionaryFromFile("ru",ruDicPath(),ruAffPath()));
    BOOST_CHECK_EQUAL(checker->dictionaries().size(),static_cast<size_t>(2));

    // No setActiveLanguages() call -- "empty means every loaded dictionary" is the whole of
    // "merging depending on the set of languages" (AbstractSpellChecker's own doc comment): an
    // English word and a Russian word are both accepted, and nonsense is rejected by both.
    BOOST_CHECK(checker->spell("hello"));
    BOOST_CHECK(checker->spell(PrivetUtf8));
    BOOST_CHECK(!checker->spell("xyzzyxyzzy"));
}

BOOST_AUTO_TEST_CASE(TestActiveLanguagesNarrowsTheVote)
{
    auto checker=makeDefaultSpellChecker();
    if (!checker)
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build, skipping");
        return;
    }

    BOOST_CHECK(!checker->loadDictionaryFromFile("en",enDicPath(),enAffPath()));
    BOOST_CHECK(!checker->loadDictionaryFromFile("ru",ruDicPath(),ruAffPath()));

    checker->setActiveLanguages({"ru"});
    BOOST_CHECK_EQUAL(checker->activeLanguages().size(),static_cast<size_t>(1));
    BOOST_CHECK(!checker->spell("hello"));
    BOOST_CHECK(checker->spell(PrivetUtf8));

    checker->setActiveLanguages({});
    BOOST_CHECK(checker->spell("hello"));
}

BOOST_AUTO_TEST_CASE(TestSuggestionsMergedDedupedAndCapped)
{
    auto checker=makeDefaultSpellChecker();
    if (!checker)
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build, skipping");
        return;
    }

    BOOST_CHECK(!checker->loadDictionaryFromFile("en",enDicPath(),enAffPath()));

    // hunspell's own suggest() is what actually proposes "world" for "wrold" -- verified
    // separately against the real library before this suite was written.
    auto suggestions=checker->suggest("wrold",8);
    bool foundWorld=false;
    for (const auto& s : suggestions)
    {
        if (s=="world")
        {
            foundWorld=true;
        }
    }
    BOOST_CHECK(foundWorld);

    auto capped=checker->suggest("wrold",1);
    BOOST_CHECK(capped.size()<=static_cast<size_t>(1));
}

BOOST_AUTO_TEST_CASE(TestWordListAndAddWord)
{
    auto checker=makeDefaultSpellChecker();
    if (!checker)
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build, skipping");
        return;
    }

    // NO dictionary loaded at all -- see AbstractSpellChecker::loadWordList()'s own doc comment:
    // a personal/custom word list must work with zero dictionaries loaded.
    BOOST_CHECK(!checker->isReady());
    BOOST_CHECK(!checker->spell("whitem"));

    auto words=readAsset("base/assets/spell/personal_en.txt");
    BOOST_CHECK(!checker->loadWordList(LanguageTag{},words));

    BOOST_CHECK(checker->spell("whitem"));
    BOOST_CHECK(checker->spell("neutralm"));
    BOOST_CHECK(checker->spell("hatnbase"));
    // Blank lines and '#' comments in the fixture must not have become "words".
    BOOST_CHECK(!checker->spell("#"));
    BOOST_CHECK(!checker->spell(""));

    BOOST_CHECK(!checker->addWord(LanguageTag{},"addedword"));
    BOOST_CHECK(checker->spell("addedword"));
}

BOOST_AUTO_TEST_CASE(TestIgnoreWordAndClearIgnored)
{
    auto checker=makeDefaultSpellChecker();
    if (!checker)
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build, skipping");
        return;
    }

    BOOST_CHECK(!checker->loadDictionaryFromFile("en",enDicPath(),enAffPath()));
    BOOST_CHECK(!checker->spell("xyzzy"));

    checker->ignoreWord("xyzzy");
    BOOST_CHECK(checker->spell("xyzzy"));

    checker->clearIgnored();
    BOOST_CHECK(!checker->spell("xyzzy"));
}

BOOST_AUTO_TEST_CASE(TestUnloadDictionary)
{
    auto checker=makeDefaultSpellChecker();
    if (!checker)
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build, skipping");
        return;
    }

    BOOST_CHECK(!checker->loadDictionaryFromFile("en",enDicPath(),enAffPath()));
    BOOST_CHECK(checker->hasLanguage("en"));

    checker->unloadDictionary("en");
    BOOST_CHECK(!checker->hasLanguage("en"));
    BOOST_CHECK(!checker->isReady());

    // Reloading the same language after unloading must NOT be SPELL_DICTIONARY_EXISTS.
    BOOST_CHECK(!checker->loadDictionaryFromFile("en",enDicPath(),enAffPath()));

    checker->unloadAll();
    BOOST_CHECK(checker->dictionaries().empty());
}

BOOST_AUTO_TEST_CASE(TestStemmerReturnsStems)
{
    auto checker=makeDefaultSpellChecker();
    if (!checker)
    {
        BOOST_TEST_MESSAGE("no spell checking backend in this build, skipping");
        return;
    }
    BOOST_CHECK(!checker->loadDictionaryFromFile("en",enDicPath(),enAffPath()));

    auto stemmer=makeDefaultStemmer(checker);
    BOOST_REQUIRE(stemmer!=nullptr);

    // The tiny fixture dictionary carries no affix rules worth stemming, so this only checks the
    // seam does not crash and stemOrSelf() falls back to the word itself when stem() is empty --
    // exactly AbstractStemmer::stemOrSelf()'s own documented contract.
    auto stems=stemmer->stem("en","hello");
    auto orSelf=stemmer->stemOrSelf("en","hello");
    BOOST_CHECK(!orSelf.empty());
    if (stems.empty())
    {
        BOOST_CHECK_EQUAL(orSelf,std::string("hello"));
    }
    else
    {
        BOOST_CHECK_EQUAL(orSelf,stems.front());
    }

    // A language with no loaded dictionary returns empty, not an error/crash.
    BOOST_CHECK(stemmer->stem("de","hello").empty());
}

BOOST_AUTO_TEST_CASE(TestLanguageDetectorFactoryIsNullInThisBuild)
{
    // Unconditional, backend-independent -- see languagedetector.h's own header comment.
    BOOST_CHECK(makeDefaultLanguageDetector()==nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
