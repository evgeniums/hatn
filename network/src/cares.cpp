/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/asio/cares.cpp
  *
  *   DNS resolver that uses c-ares resolving library
  *
  */

/****************************************************************************/

#include <c-ares/ares.h>

#include <hatn/common/thread.h>

#include <hatn/network/error.h>

#include <hatn/network/dns/cares.h>

namespace hatn {

using namespace common;

namespace network {
namespace dns {

/********************** CaresLib **************************/

pmr::AllocatorFactory* CaresLib::m_allocatorFactory=nullptr;
static std::unique_ptr<common::pmr::polymorphic_allocator<char>> Allocator;

static void* myMalloc(size_t size)
{
    size+=sizeof(size_t);
    auto buf=Allocator->allocate(size);
    auto sizeVal=reinterpret_cast<size_t*>(buf);
    *sizeVal=size;
    return (buf+sizeof(size_t));
}
static void myFree(void* ptr)
{
    auto buf=reinterpret_cast<char*>(ptr)-sizeof(size_t);
    auto size=reinterpret_cast<size_t*>(buf);
    Allocator->deallocate(buf,*size);
}
static void* myRealloc(void *ptr, size_t size)
{
    auto newSize=size+sizeof(size_t);
    auto buf=Allocator->allocate(newSize);
    auto prevBuf=reinterpret_cast<char*>(ptr)-sizeof(size_t);
    auto prevSize=reinterpret_cast<size_t*>(prevBuf);
    auto copySize=(std::min)(size,(*prevSize-sizeof(size_t)));
    memcpy(buf+sizeof(size_t),ptr,copySize);
    auto sizeVal=reinterpret_cast<size_t*>(buf);
    *sizeVal=newSize;
    Allocator->deallocate(prevBuf,*prevSize);
    return (buf+sizeof(size_t));
}

//---------------------------------------------------------------
common::Error CaresLib::init(pmr::AllocatorFactory *allocatorFactory)
{
    int res=ARES_SUCCESS;
    if (allocatorFactory!=nullptr)
    {
        m_allocatorFactory=allocatorFactory;
        Allocator=std::move(lib::make_unique<common::pmr::polymorphic_allocator<char>>(allocatorFactory->dataMemoryResource()));
        res=ares_library_init_mem(ARES_LIB_INIT_ALL,myMalloc,myFree,myRealloc);
    }
    else
    {
        res=ares_library_init(ARES_LIB_INIT_ALL);
    }
    if (res!=ARES_SUCCESS)
    {
        return makeCaresError(res);
    }
    return common::Error();
}

//---------------------------------------------------------------
void CaresLib::cleanup()
{
    ares_library_cleanup();
    Allocator.reset();
    m_allocatorFactory=nullptr;
}

/********************** CaresError **************************/

//---------------------------------------------------------------
std::string CaresError::message() const
{
    return std::string(ares_strerror(m_code));
}

//---------------------------------------------------------------
bool CaresError::isNull() const noexcept
{
    return m_code==ARES_SUCCESS;
}

//---------------------------------------------------------------
} // namespace dns
} // namespace network
} // namespace hatn
