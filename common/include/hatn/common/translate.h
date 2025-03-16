/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/translate.h
 *
 *     Strings localization classes
 */
/****************************************************************************/

#ifndef HATNTRANSLATE_H
#define HATNTRANSLATE_H

#include <string>
#include <vector>
#include <memory>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Base class for translators
class HATN_COMMON_EXPORT Translator
{
    public:

        Translator()=default;
        virtual ~Translator()=default;
        Translator(const Translator&)=default;
        Translator(Translator&&) =default;
        Translator& operator=(const Translator&)=default;
        Translator& operator=(Translator&&) =default;

        /**
         * @brief Translate a string
         * @param phrase String to translate
         * @param context Context of the translation
         * @return Translated string
         */
        virtual std::string translate(
            const std::string& phrase,
            const std::string& context=std::string("generic")
        ) const;

        /**
         * @brief Get application translator
         * @return Current application translator
         */
        static const std::shared_ptr<Translator>& translator() noexcept;

        /**
         * @brief Set application translator
         * @param translator Global application translator
         */
        static void setTranslator(std::shared_ptr<Translator> translator) noexcept;
};

class BoostTranslatorFactory_p;
/**
 * @brief Translator factory that uses boost::locale to perform translations.
 *
 * Works with UTF-8 encoding an all platforms.
 * On Windows OEM encodings can be used also.
 */
class HATN_COMMON_EXPORT BoostTranslatorFactory final
{
    public:

        //! Ctor
        BoostTranslatorFactory();

        ~BoostTranslatorFactory();
        BoostTranslatorFactory(const BoostTranslatorFactory&) =delete;
        BoostTranslatorFactory(BoostTranslatorFactory&&) noexcept;
        BoostTranslatorFactory& operator=(const BoostTranslatorFactory&) =delete;
        BoostTranslatorFactory& operator=(BoostTranslatorFactory&&) noexcept;

        /**
         * @brief Add path where to look for dictionaries
         * @param path Folder with dictionaries
         */
        void addDictionaryPath(const std::string &path);

        /**
         * @brief Load default dictionary paths
         *
         * On Linux: ./locale, /usr/locale, /usr/share/locale, /usr/local/locale, /usr/local/share/locale
         * On Windows: ./locale
         */
        void loadDefaultDictionaryPaths();

        /**
         * @brief Add message domain to use for lookups
         * @param domain Mesage domain as defined in boost.locale, in practice it is a name of .mo file without extension
         */
        void addMessagesDomain(const std::string &domain);

        enum class WinConvertOEM : int
        {
            Auto,
            Enable,
            Disable
        };

        /**
         * @brief Create translator for locale
         * @param locale Locale
         * @param utf8Enc Use UTF-8 encoding, meaningful only on Windows, ignored on other platforms
         * @param convertOEM Convertion mode of Windows OEM encoding
         * @return Translator
         */
        std::shared_ptr<Translator> create(const std::string& locale,
                                           bool utf8Enc=true,
                                           const WinConvertOEM& convertOEM=WinConvertOEM::Auto
                                        ) const;

        /**
         * @brief Create translator for locale
         * @param locale Locale
         * @param utf8Enc Use UTF-8 encoding, meaningful only on Windows, ignored on other platforms
         * @param convertOEM Convertion mode of Windows OEM encoding
         * @return Translator
         */
        inline std::shared_ptr<Translator> create(const char* locale,
                                           bool utf8Enc=true,
                                           const WinConvertOEM& convertOEM=WinConvertOEM::Auto
                                        ) const
        {
            return create(std::string(locale),utf8Enc,convertOEM);
        }

        /**
         * @brief Create translator for default locale
         * @param utf8Enc Use UTF-8 encoding, meaningful only on Windows, ignored on other platforms
         * @param convertOEM Convertion mode of Windows OEM encoding
         * @return Translator
         */
        inline std::shared_ptr<Translator> create(
                                           bool utf8Enc=true,
                                           const WinConvertOEM& convertOEM=WinConvertOEM::Auto
                                        ) const
        {
            return create(std::string(),utf8Enc,convertOEM);
        }

    private:

        std::unique_ptr<BoostTranslatorFactory_p> d;
};

HATN_COMMON_NAMESPACE_END

HATN_NAMESPACE_BEGIN

    /**
 * @brief Mark string for further localization
 * @param phrase String that will be translated
 * @param context Context of the translation
 * @return Translated string
 */
    inline std::string _TR(
        const std::string& phrase,
        const std::string& context="generic"
        )
{
    const auto& translator=common::Translator::translator();
    if (translator)
    {
        return translator->translate(phrase,context);
    }
    return phrase;
}

inline std::string _TR(
    const std::string& phrase,
    const std::string& context,
    const common::Translator* translator
    )
{
    if (translator!=nullptr)
    {
        return translator->translate(phrase,context);
    }
    const auto& localeTr=common::Translator::translator();
    if (localeTr)
    {
        return localeTr->translate(phrase,context);
    }
    return phrase;
}

HATN_NAMESPACE_END

#endif // HATNTRANSLATE_H
