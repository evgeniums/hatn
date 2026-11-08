#include <boost/test/unit_test.hpp>

#include <hatn/common/utf8_tokenize.h>

HATN_USING
HATN_COMMON_USING

#ifdef HATN_USE_UTF8PROC

BOOST_AUTO_TEST_SUITE(TestUtf8Tokenize)

namespace {

std::vector<std::string> words(const std::vector<Utf8Token>& tokens)
{
    std::vector<std::string> result;
    for (const auto& token : tokens)
    {
        result.push_back(token.token);
    }
    return result;
}

}

BOOST_AUTO_TEST_CASE(OffsetsPointIntoTheInput)
{
    Utf8Proc proc;
    const std::string text="  Hello, wörld!  Привет мир ";
    auto tokens=proc.tokenizeWithOffsets(text);

    BOOST_REQUIRE_EQUAL(tokens.size(),4u);
    for (const auto& token : tokens)
    {
        BOOST_CHECK_EQUAL(text.substr(token.offset,token.length),token.token);
    }
    std::vector<std::string> expected{"Hello","wörld","Привет","мир"};
    BOOST_CHECK(words(tokens)==expected);
}

BOOST_AUTO_TEST_CASE(WordsAreNotNormalized)
{
    Utf8Proc proc;
    // normalization would turn the "й" into "и" and lower-case the text; the raw word must survive
    auto tokens=proc.tokenizeWithOffsets("Мой Éclair");
    std::vector<std::string> expected{"Мой","Éclair"};
    BOOST_CHECK(words(tokens)==expected);
}

BOOST_AUTO_TEST_CASE(PunctuationSplitsByDefault)
{
    Utf8Proc proc;
    std::vector<std::string> plain{"see","example","com","and","a","b"};
    BOOST_CHECK(words(proc.tokenizeWithOffsets("see example.com and a-b"))==plain);

    std::vector<std::string> domains{"see","example.com","and","a-b"};
    BOOST_CHECK(words(proc.tokenizeWithOffsets("see example.com and a-b",true))==domains);

    // glue needs a letter or a digit on both sides
    auto tokens=proc.tokenizeWithOffsets("end. -x.",true);
    std::vector<std::string> wanted{"end","x"};
    BOOST_CHECK(words(tokens)==wanted);
}

BOOST_AUTO_TEST_CASE(InvalidBytesAreBoundaries)
{
    Utf8Proc proc;
    std::string text="ab";
    text.push_back(static_cast<char>(0xFF));
    text+="cd";
    auto tokens=proc.tokenizeWithOffsets(text);
    BOOST_REQUIRE_EQUAL(tokens.size(),2u);
    BOOST_CHECK_EQUAL(tokens[0].token,"ab");
    BOOST_CHECK_EQUAL(tokens[1].token,"cd");
    BOOST_CHECK_EQUAL(tokens[1].offset,3u);
}

BOOST_AUTO_TEST_CASE(EmptyAndSeparatorsOnly)
{
    Utf8Proc proc;
    BOOST_CHECK(proc.tokenizeWithOffsets("").empty());
    BOOST_CHECK(proc.tokenizeWithOffsets(" \t\n ,.;").empty());
}

BOOST_AUTO_TEST_SUITE_END()

#endif
