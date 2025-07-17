/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/randomgenerator.h
 *
 *  Random data generator
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTRANDOMGENERATOR_H
#define HATNCRYPTRANDOMGENERATOR_H

#include <hatn/common/error.h>
#include <hatn/common/memorylockeddata.h>

#include <hatn/crypt/crypt.h>

HATN_CRYPT_NAMESPACE_BEGIN

/**
 * @brief The RandomGenerator class
 */
class RandomGenerator
{
    public:

        RandomGenerator()=default;
        virtual ~RandomGenerator()=default;
        RandomGenerator(const RandomGenerator&)=delete;
        RandomGenerator(RandomGenerator&&) =delete;
        RandomGenerator& operator=(const RandomGenerator&)=delete;
        RandomGenerator& operator=(RandomGenerator&&) =delete;

        virtual common::Error randBytes(char*,size_t) const =0;

        template <typename ContainerT>
        common::Error randContainer(ContainerT& container, size_t maxSize, size_t minSize=0) const
        {
            if (maxSize==0)
            {
                container.clear();
                return common::Error();
            }
            size_t sizeDiff=0;
            container.resize(maxSize+sizeof(sizeDiff));

            auto ec=randBytes(container.data(),container.size());
            if (ec)
            {
                container.clear();
            }
            memcpy(&sizeDiff,container.data()+container.size()-sizeof(sizeDiff),sizeof(sizeDiff));
            auto size=maxSize;
            if (minSize!=0 && minSize<maxSize)
            {
                size=minSize+sizeDiff%(maxSize-minSize);
            }
            container.resize(size);
            return OK;
        }
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTRANDOMGENERATOR_H
