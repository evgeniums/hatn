/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file base/hunspellchecker.cpp
  *
  * Defines HunspellChecker/HunspellStemmer.
  *
  */

/****************************************************************************/

// The vendor include lives HERE, behind the same #ifdef the whole header is already behind --
// never outside it. common/src/utf8_tokenize.cpp puts <utf8proc.h> outside its own
// HATN_USE_UTF8PROC guard, which breaks that one translation unit whenever the dependency is
// absent; this file does not repeat that.
#include <hatn/base/hunspellchecker.h>

#ifdef HATN_USE_HUNSPELL

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#include <hunspell/hunspell.hxx>

#include <hatn/base/baseerror.h>

HATN_BASE_NAMESPACE_BEGIN

namespace {

std::atomic<unsigned long> TempFileCounter{0};

std::string systemTempDir()
{
#ifdef _WIN32
    const char* dir=std::getenv("TEMP");
    if (dir==nullptr)
    {
        dir=std::getenv("TMP");
    }
    return (dir!=nullptr) ? std::string(dir) : std::string(".");
#else
    const char* dir=std::getenv("TMPDIR");
    return (dir!=nullptr) ? std::string(dir) : std::string("/tmp");
#endif
}

//! A path that no other call to this function (in this process) has returned before.
std::string uniqueTempPath(const char* suffix)
{
    auto dir=systemTempDir();
    if (!dir.empty() && dir.back()!='/' && dir.back()!='\\')
    {
        dir+='/';
    }
    const auto n=TempFileCounter.fetch_add(1,std::memory_order_relaxed);
    std::ostringstream name;
    name << dir << "hatn-spell-" << static_cast<const void*>(&TempFileCounter) << '-' << n << suffix;
    return name.str();
}

bool writeWholeFile(const std::string& path, common::lib::string_view content)
{
    FILE* f=std::fopen(path.c_str(),"wb");
    if (f==nullptr)
    {
        return false;
    }
    const auto written=std::fwrite(content.data(),1,content.size(),f);
    std::fclose(f);
    return written==content.size();
}

//! Best-effort delete -- a leftover temp file is a nuisance, not a correctness problem, so this
//! is never treated as an error by any caller.
void removeIfExists(const std::string& path)
{
    if (!path.empty())
    {
        std::remove(path.c_str());
    }
}

bool isUtf8Encoding(const std::string& encoding)
{
    static const std::string Utf8("UTF-8");
    if (encoding.size()!=Utf8.size())
    {
        return false;
    }
    return std::equal(encoding.begin(),encoding.end(),Utf8.begin(),
        [](char a, char b)
        {
            return std::toupper(static_cast<unsigned char>(a))==b;
        }
    );
}

//! Hunspell always needs an affix file -- there is no "no affix" mode. An empty `affPath`
//! (AbstractSpellChecker::loadDictionaryFromFile()'s documented convention for "derive it")
//! falls back to the conventional sibling ".aff" next to the ".dic", the shape every shipped
//! hunspell dictionary takes.
std::string deriveAffPath(const std::string& dicPath)
{
    auto path=dicPath;
    static const std::string DicSuffix(".dic");
    if (path.size()>=DicSuffix.size()
        && path.compare(path.size()-DicSuffix.size(),DicSuffix.size(),DicSuffix)==0)
    {
        path.replace(path.size()-DicSuffix.size(),DicSuffix.size(),".aff");
    }
    else
    {
        path+=".aff";
    }
    return path;
}

//! Split a personal word list on lines, trimming and dropping blanks/#comments -- see
//! AbstractSpellChecker::loadWordList().
std::vector<std::string> splitWordList(common::lib::string_view words)
{
    std::vector<std::string> result;

    size_t pos=0;
    while (pos<=words.size())
    {
        auto end=words.find('\n',pos);
        if (end==common::lib::string_view::npos)
        {
            end=words.size();
        }
        auto line=words.substr(pos,end-pos);
        // Trim a trailing '\r' (CRLF line endings) and surrounding whitespace.
        while (!line.empty() && (line.back()=='\r' || line.back()==' ' || line.back()=='\t'))
        {
            line.remove_suffix(1);
        }
        while (!line.empty() && (line.front()==' ' || line.front()=='\t'))
        {
            line.remove_prefix(1);
        }
        if (!line.empty() && line.front()!='#')
        {
            result.emplace_back(line);
        }
        pos=end+1;
    }

    return result;
}

}

//---------------------------------------------------------------

HunspellChecker::HunspellChecker()
{}

//---------------------------------------------------------------

HunspellChecker::~HunspellChecker()
{
    // Best-effort: cleans up only the temp files a Windows loadDictionaryFromBuffer() call could
    // not remove immediately (the file was still open when construction returned) -- see that
    // method's own doc comment. A no-op everywhere else, since tempDicPath/tempAffPath are empty
    // whenever the immediate removal already succeeded.
    for (auto& dict : m_dicts)
    {
        removeIfExists(dict.tempDicPath);
        removeIfExists(dict.tempAffPath);
    }
}

//---------------------------------------------------------------

std::vector<LanguageTag> HunspellChecker::iterationOrderLocked() const
{
    if (!m_activeLanguages.empty())
    {
        return m_activeLanguages;
    }
    std::vector<LanguageTag> order;
    order.reserve(m_dicts.size());
    for (const auto& dict : m_dicts)
    {
        order.push_back(dict.language);
    }
    return order;
}

//---------------------------------------------------------------

HunspellChecker::Dict* HunspellChecker::dictForLocked(const LanguageTag& language)
{
    for (auto& dict : m_dicts)
    {
        if (dict.language==language)
        {
            return &dict;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------

const HunspellChecker::Dict* HunspellChecker::dictForLocked(const LanguageTag& language) const
{
    for (const auto& dict : m_dicts)
    {
        if (dict.language==language)
        {
            return &dict;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------

bool HunspellChecker::isPersonalWordLocked(const LanguageTag& language, const std::string& word) const
{
    auto global=m_personalWords.find(LanguageTag{});
    if (global!=m_personalWords.end() && global->second.count(word)!=0)
    {
        return true;
    }
    if (language.empty())
    {
        return false;
    }
    auto it=m_personalWords.find(language);
    return it!=m_personalWords.end() && it->second.count(word)!=0;
}

//---------------------------------------------------------------

Error HunspellChecker::loadDictionaryFromFile(
        const LanguageTag& language,
        common::lib::string_view dicPath,
        common::lib::string_view affPath
    )
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (dictForLocked(language)!=nullptr)
    {
        return baseError(BaseError::SPELL_DICTIONARY_EXISTS);
    }

    const std::string dic(dicPath);
    const std::string aff=affPath.empty() ? deriveAffPath(dic) : std::string(affPath);

    auto handle=std::make_unique<Hunspell>(aff.c_str(),dic.c_str());
    if (handle->get_dict_encoding().empty())
    {
        // A Hunspell instance that could not open its aff/dic files still exists (the ctor
        // itself does not throw or return a status), but reports no encoding at all -- the
        // closest thing to a load-failure signal this API offers.
        return baseError(BaseError::SPELL_DICTIONARY_LOAD_ERROR);
    }
    if (!isUtf8Encoding(handle->get_dict_encoding()))
    {
        return baseError(BaseError::SPELL_DICTIONARY_ENCODING);
    }

    Dict entry;
    entry.language=language;
    entry.name=dic;
    entry.handle=std::move(handle);
    m_dicts.push_back(std::move(entry));
    return OK;
}

//---------------------------------------------------------------

Error HunspellChecker::loadDictionaryFromBuffer(
        const LanguageTag& language,
        common::lib::string_view dic,
        common::lib::string_view aff
    )
{
    // Hunspell has no in-memory constructor -- see this method's own doc comment in
    // hunspellchecker.h. Unlike loadDictionaryFromFile(), an empty `aff` has no sibling-file
    // convention to fall back to here (a buffer has no path at all), so it is an error rather
    // than a derived default.
    if (aff.empty())
    {
        return baseError(BaseError::SPELL_DICTIONARY_LOAD_ERROR);
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (dictForLocked(language)!=nullptr)
    {
        return baseError(BaseError::SPELL_DICTIONARY_EXISTS);
    }

    const auto dicPath=uniqueTempPath(".dic");
    const auto affPath=uniqueTempPath(".aff");

    if (!writeWholeFile(dicPath,dic) || !writeWholeFile(affPath,aff))
    {
        removeIfExists(dicPath);
        removeIfExists(affPath);
        return baseError(BaseError::SPELL_DICTIONARY_LOAD_ERROR);
    }

    auto handle=std::make_unique<Hunspell>(affPath.c_str(),dicPath.c_str());
    const auto encoding=handle->get_dict_encoding();
    if (encoding.empty())
    {
        removeIfExists(dicPath);
        removeIfExists(affPath);
        return baseError(BaseError::SPELL_DICTIONARY_LOAD_ERROR);
    }
    if (!isUtf8Encoding(encoding))
    {
        removeIfExists(dicPath);
        removeIfExists(affPath);
        return baseError(BaseError::SPELL_DICTIONARY_ENCODING);
    }

    Dict loaded;
    loaded.language=language;
    loaded.name="<buffer>";
    loaded.handle=std::move(handle);

    // Removed immediately -- Hunspell reads both files fully during construction and does not
    // keep them open afterward. If removal fails (Windows: still momentarily open), the path is
    // kept and retried from the destructor -- see that method's own comment.
    if (std::remove(dicPath.c_str())!=0)
    {
        loaded.tempDicPath=dicPath;
    }
    if (std::remove(affPath.c_str())!=0)
    {
        loaded.tempAffPath=affPath;
    }

    m_dicts.push_back(std::move(loaded));
    return OK;
}

//---------------------------------------------------------------

Error HunspellChecker::loadWordList(const LanguageTag& language, common::lib::string_view words)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto& bucket=m_personalWords[language];
    for (auto&& word : splitWordList(words))
    {
        bucket.insert(std::move(word));
    }
    return OK;
}

//---------------------------------------------------------------

void HunspellChecker::unloadDictionary(const LanguageTag& language)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto it=m_dicts.begin(); it!=m_dicts.end(); ++it)
    {
        if (it->language==language)
        {
            removeIfExists(it->tempDicPath);
            removeIfExists(it->tempAffPath);
            m_dicts.erase(it);
            break;
        }
    }
}

//---------------------------------------------------------------

void HunspellChecker::unloadAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& dict : m_dicts)
    {
        removeIfExists(dict.tempDicPath);
        removeIfExists(dict.tempAffPath);
    }
    m_dicts.clear();
}

//---------------------------------------------------------------

std::vector<DictionaryInfo> HunspellChecker::dictionaries() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<DictionaryInfo> result;
    result.reserve(m_dicts.size()+m_personalWords.size());
    for (const auto& dict : m_dicts)
    {
        result.push_back(DictionaryInfo{dict.language,dict.name,false});
    }
    for (const auto& bucket : m_personalWords)
    {
        if (!bucket.second.empty())
        {
            result.push_back(DictionaryInfo{bucket.first,"personal",true});
        }
    }
    return result;
}

//---------------------------------------------------------------

void HunspellChecker::setActiveLanguages(std::vector<LanguageTag> languages)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeLanguages=std::move(languages);
}

//---------------------------------------------------------------

const std::vector<LanguageTag>& HunspellChecker::activeLanguages() const
{
    // NOT locked: returns a reference into this object's own state, matching
    // ConfigTreeIo::formats()'s identical (unlocked, noexcept) shape for a stable member -- the
    // caller is responsible for not racing setActiveLanguages() on another thread, same as any
    // other reference-returning accessor here.
    return m_activeLanguages;
}

//---------------------------------------------------------------

Error HunspellChecker::addWord(const LanguageTag& language, common::lib::string_view word)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_personalWords[language].emplace(word);
    return OK;
}

//---------------------------------------------------------------

void HunspellChecker::ignoreWord(common::lib::string_view word)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ignoredWords.emplace(word);
}

//---------------------------------------------------------------

void HunspellChecker::clearIgnored()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ignoredWords.clear();
}

//---------------------------------------------------------------

bool HunspellChecker::spell(common::lib::string_view word) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const std::string w(word);
    if (m_ignoredWords.count(w)!=0)
    {
        return true;
    }

    // Checked UNCONDITIONALLY, before the per-language loop below: the global ("") personal-word
    // bucket is a cross-language custom dictionary by design (AbstractSpellChecker::
    // loadWordList()'s own doc comment), and MUST work with zero dictionaries loaded and zero
    // active languages set -- iterationOrderLocked() is empty in exactly that state, so a global
    // word would otherwise never be reached at all.
    if (isPersonalWordLocked(LanguageTag{},w))
    {
        return true;
    }

    for (const auto& language : iterationOrderLocked())
    {
        if (isPersonalWordLocked(language,w))
        {
            return true;
        }
        const auto* dict=dictForLocked(language);
        if (dict!=nullptr && dict->handle->spell(w))
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------

LanguageTag HunspellChecker::spellLanguage(common::lib::string_view word) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const std::string w(word);
    if (m_ignoredWords.count(w)!=0)
    {
        return LanguageTag{};
    }

    // See the identical check in spell() above.
    if (isPersonalWordLocked(LanguageTag{},w))
    {
        return LanguageTag{};
    }

    for (const auto& language : iterationOrderLocked())
    {
        if (isPersonalWordLocked(language,w))
        {
            return language;
        }
        const auto* dict=dictForLocked(language);
        if (dict!=nullptr && dict->handle->spell(w))
        {
            return language;
        }
    }
    return LanguageTag{};
}

//---------------------------------------------------------------

std::vector<std::string> HunspellChecker::suggest(common::lib::string_view word, size_t maxCount) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::string> result;
    std::unordered_set<std::string> seen;

    const std::string w(word);
    for (const auto& language : iterationOrderLocked())
    {
        const auto* dict=dictForLocked(language);
        if (dict==nullptr)
        {
            continue;
        }
        for (auto&& suggestion : dict->handle->suggest(w))
        {
            if (result.size()>=maxCount)
            {
                return result;
            }
            if (seen.insert(suggestion).second)
            {
                result.push_back(std::move(suggestion));
            }
        }
    }
    return result;
}

//---------------------------------------------------------------

HunspellStemmer::HunspellStemmer(std::shared_ptr<HunspellChecker> checker)
    : m_checker(std::move(checker))
{}

//---------------------------------------------------------------

std::vector<std::string> HunspellStemmer::stem(const LanguageTag& language, common::lib::string_view word) const
{
    std::lock_guard<std::mutex> lock(m_checker->m_mutex);

    const auto* dict=m_checker->dictForLocked(language);
    if (dict==nullptr)
    {
        return {};
    }
    return dict->handle->stem(std::string(word));
}

//---------------------------------------------------------------

std::vector<std::string> HunspellStemmer::analyze(const LanguageTag& language, common::lib::string_view word) const
{
    std::lock_guard<std::mutex> lock(m_checker->m_mutex);

    const auto* dict=m_checker->dictForLocked(language);
    if (dict==nullptr)
    {
        return {};
    }
    return dict->handle->analyze(std::string(word));
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END

#endif // HATN_USE_HUNSPELL
