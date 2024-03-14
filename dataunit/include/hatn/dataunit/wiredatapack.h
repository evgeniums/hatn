/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/wiredatapack.h
  *
  *      Wire dataunit pack.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITWIREDATAPACK_H
#define HATNDATAUNITWIREDATAPACK_H

HATN_DATAUNIT_NAMESPACE_BEGIN

#ifdef HATN_WIREDATA_SRC
HATN_WITH_STATIC_ALLOCATOR_IMPL
#endif

//! Wire dataunit pack
class HATN_DATAUNIT_EXPORT WireDataPack
{
    public:

        virtual WireDataSingle* single() noexcept
        {
            return nullptr;
        }
        virtual WireDataSingleShared* shared() noexcept
        {
            return nullptr;
        }
        virtual WireDataChained* chained() noexcept
        {
            return nullptr;
        }

        virtual WireData* wireData() noexcept =0;

        WireDataPack()=default;
        virtual ~WireDataPack()=default;
        WireDataPack(const WireDataPack&)=default;
        WireDataPack(WireDataPack&&) =default;
        WireDataPack& operator=(const WireDataPack&)=default;
        WireDataPack& operator=(WireDataPack&&) =default;
};

HATN_WITH_STATIC_ALLOCATOR_DECLARE(WireDataPackSingle,HATN_DATAUNIT_EXPORT)
//! Wire dataunit pack with single unit
class WireDataPackSingle : public WireDataPack,
                            public WithStaticAllocator<WireDataPackSingle>
{
    public:

        virtual WireDataSingle* single() noexcept override
        {
            return &data;
        }

        virtual WireData* wireData() noexcept override
        {
            return &data;
        }

    private:

        WireDataSingle data;
};

HATN_WITH_STATIC_ALLOCATOR_DECLARE(WireDataPackSingleShared,HATN_DATAUNIT_EXPORT)
//! Wire dataunit pack with shared unit
class WireDataPackSingleShared : public WireDataPack,
                                  public WithStaticAllocator<WireDataPackSingleShared>
{
    public:

        virtual WireDataSingleShared* shared() noexcept override
        {
            return &data;
        }

        virtual WireData* wireData() noexcept override
        {
            return &data;
        }

    private:

        WireDataSingleShared data;
};

HATN_WITH_STATIC_ALLOCATOR_DECLARE(WireDataPackChained,HATN_DATAUNIT_EXPORT)
//! Wire dataunit pack with chained unit
class WireDataPackChained : public WireDataPack,
                             public WithStaticAllocator<WireDataPackSingleShared>
{
    public:

        virtual WireDataChained* chained() noexcept override
        {
            return &data;
        }

        virtual WireData* wireData() noexcept override
        {
            return &data;
        }

    private:

        WireDataChained data;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITWIREDATAPACK_H
