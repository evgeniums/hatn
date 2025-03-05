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
    using random_bytes_engine = std::independent_bits_engine<
        std::default_random_engine, CHAR_BIT, uint16_t>;
    random_bytes_engine rbe;
    auto b=reinterpret_cast<uint16_t*>(buf);
    std::generate(b, b+size, std::ref(rbe));
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
