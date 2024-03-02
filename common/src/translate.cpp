/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/translate.cpp
 *
 *     Strings localization classes
 *
 */
/****************************************************************************/

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

#include <boost/locale.hpp>
#ifdef WIN32
#include <windows.h>
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

#include <hatn/common/types.h>
#include <hatn/common/format.h>
#include <hatn/common/translate.h>

HATN_COMMON_NAMESPACE_BEGIN

/********************** Translator **********************************/

static std::shared_ptr<Translator> TranslatorInst;

//---------------------------------------------------------------
std::string Translator::translate(
        const std::string &phrase,
        const std::string &
    ) const
{
    return phrase;
}

//---------------------------------------------------------------
const std::shared_ptr<Translator>& Translator::translator() noexcept
{
    return TranslatorInst;
}

//---------------------------------------------------------------
void Translator::setTranslator(std::shared_ptr<Translator> translator) noexcept
{
    TranslatorInst = std::move(translator);
}

namespace
{

/********************** BoostTranslator **********************************/

/**
 * @brief Translator that uses boost::locale to perform translations.
 *
 * Works with UTF-8 encoding an all platforms.
 * On Windows OEM encodings can be used also.
 */
class BoostTranslator : public Translator
{
    public:

        BoostTranslator(const std::locale& locale):m_locale(locale)
        {}

        virtual std::string translate(
            const std::string& phrase,
            const std::string& context
        ) const override;

        std::locale m_locale;
        std::vector<std::string> m_domains;

#ifdef WIN32

        inline std::string fromUtf8(const std::string &utf) const
        {
            return boost::locale::conv::from_utf(utf, fmt::format("CP{0}",GetACP()));
        }

        bool m_convertOem=false;
        bool m_utf8=true;
#endif
};

//---------------------------------------------------------------
std::string BoostTranslator::translate(
    const std::string& phrase,
    const std::string& context
) const
{
    boost::locale::basic_message<char> tr;
    if (context != "generic")
    {
        tr = boost::locale::translate(context, phrase);
    }
    else
    {
        tr = boost::locale::translate(phrase);
    }

    std::string r = phrase;
    for (auto&& domain : m_domains)
    {
        std::string s = tr.str(m_locale, domain);
        if (s != phrase)
        {
            r = s;
            break;
        }
    }

#ifdef WIN32
    if (m_utf8)
    {
        return r;
    }

    r = fromUtf8(r);
    if (!m_convertOem)
    {
        return r;
    }

    std::vector<char> rr(r.begin(), r.end());
    rr.push_back(0);
    ::CharToOemA(r.c_str(), rr.data());

    return std::string(rr.data());
#else
    return r;
#endif
}
}

/********************** BoostTranslatorFactory **********************************/

class BoostTranslatorFactory_p
{
    public:

        boost::locale::generator generator;
        std::vector<std::string> domains;
};

//---------------------------------------------------------------
BoostTranslatorFactory::BoostTranslatorFactory():d(std::make_unique<BoostTranslatorFactory_p>())
{
}

BoostTranslatorFactory::~BoostTranslatorFactory()=default;
BoostTranslatorFactory::BoostTranslatorFactory(BoostTranslatorFactory&&) noexcept=default;
BoostTranslatorFactory& BoostTranslatorFactory::operator=(BoostTranslatorFactory&&) noexcept=default;

//---------------------------------------------------------------
void BoostTranslatorFactory::addDictionaryPath(const std::string &path)
{
    d->generator.add_messages_path(path);
}

//---------------------------------------------------------------
void BoostTranslatorFactory::loadDefaultDictionaryPaths()
{
    d->generator.add_messages_path("./locale");
#ifndef _WIN32
    d->generator.add_messages_path("/usr/locale/");
    d->generator.add_messages_path("/usr/share/locale/");
    d->generator.add_messages_path("/usr/local/locale/");
    d->generator.add_messages_path("/usr/local/share/locale/");
#endif // _WIN32
}

//---------------------------------------------------------------
void BoostTranslatorFactory::addMessagesDomain(const std::string &domain)
{
    d->generator.add_messages_domain(domain);
    d->domains.push_back(domain);
}

//---------------------------------------------------------------
std::shared_ptr<Translator> BoostTranslatorFactory::create(const std::string &loc, bool utf8Enc, const WinConvertOEM& convertOEM) const
{
    auto tr=std::make_shared<BoostTranslator>(d->generator(loc));
    tr->m_domains = d->domains;

#ifdef WIN32
    switch (convertOEM)
    {
        case (WinConvertOEM::Auto):
        {
            tr->m_convertOem = (GetConsoleCP() == GetOEMCP());
        }
            break;
        case (WinConvertOEM::Enable):
        {
            tr->m_convertOem = true;
        }
            break;
        case (WinConvertOEM::Disable):
        {
            tr->m_convertOem = false;
        }
            break;
    }
    tr->m_utf8 = utf8Enc;
#else
    std::ignore=utf8Enc;
    std::ignore=convertOEM;
#endif

    return tr;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
