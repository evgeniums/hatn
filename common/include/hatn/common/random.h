/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/random.h
  *
  *  Contains Random generators.
  *
  */

/****************************************************************************/

#ifndef HATNRANDOM_H
#define HATNRANDOM_H

#include <cassert>
#include <cstdint>

#include <boost/any.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <hatn/common/common.h>
#include <hatn/common/error.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Random numbers generators.
class HATN_COMMON_EXPORT Random
{
    public:

        /**
         * @brief Get random value uniformly distributed in range.
         * @param min Min value.
         * @param max max value.
         * @return Generated value.
         */
        static uint32_t uniform(const uint32_t& min, const uint32_t& max);

        /**
         * @brief Generate random value less than max.
         * @param max Max value.
         * @return Generated value.
         */
        static uint32_t generate(const uint32_t& max)
        {
            return uniform(0,max);
        }

        /**
         * @brief Fill buffer with random data.
         * @param buf Buffer.
         * @param size Buffer size.
         */
        static void bytes(char* buf, size_t size);

        /**
         * @brief Fill container with random data.
         * @param container Container.
         * @param maxSize Max container size.
         * @param minSize Min container size, default 0.
         */
        template <typename ContainerT>
        static void randContainer(ContainerT& container, size_t maxSize, size_t minSize=0)
        {
            auto size=uniform(static_cast<uint32_t>(minSize),static_cast<uint32_t>(maxSize));
            if (size==0)
            {
                container.clear();
                return;
            }

            container.resize(size);
            bytes(container.data(),container.size());
        }
};

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END

#endif // HATNRANDOM_H
