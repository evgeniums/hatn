/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/random.сpp
  *
  *      Random data generators.
  *
  */

#include <random>

#include <hatn/common/format.h>
#include <hatn/common/random.h>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------

uint32_t Random::uniform(const uint32_t& min, const uint32_t& max)
{
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<uint32_t> distr{min,max};
    return distr(gen);
}

//---------------------------------------------------------------

void Random::bytes(char* buf, size_t size)
{
#ifdef _MSC_VER
    using type=uint16_t;
    size=size/2;
#else
    using type=uint8_t;
#endif
    using random_bytes_engine = std::independent_bits_engine<std::mt19937, CHAR_BIT, type>;
    std::random_device rd;
    random_bytes_engine rbe(rd());
    auto b=reinterpret_cast<type*>(buf);
    std::generate(b, b+size, std::ref(rbe));
}

//---------------------------------------------------------------

std::string Random::generateAsString(const uint32_t& max, size_t minDigits)
{
    auto num=generate(max);
    auto format=fmt::format("{{:0{}d}}",minDigits);
    return fmt::format(format,num);
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
